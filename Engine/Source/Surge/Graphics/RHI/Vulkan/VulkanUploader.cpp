// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/RHI/Vulkan/VulkanUploader.hpp"
#include "Surge/Graphics/RHI/Vulkan/VulkanRHI.hpp"
#include "Surge/Core/Profiler.hpp"

namespace Surge
{
    void VulkanUploader::Initialize(const VulkanRHI& rhi)
    {
        VkCommandPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        poolInfo.queueFamilyIndex = static_cast<Uint>(rhi.GetQueueIndex());
        VK_CALL(vkCreateCommandPool(rhi.GetDevice(), &poolInfo, nullptr, &mCommandPool));
        SET_VK_DEBUG_NAME(rhi, VK_OBJECT_TYPE_COMMAND_POOL, (uint64_t)mCommandPool, "UploadCommandPool");

        mBatches.reserve(8);
        mSubmitOrder.reserve(8);
        mFreeBlocks.reserve(MAX_FREE_BLOCKS);
    }

    void VulkanUploader::Shutdown(const VulkanRHI& rhi)
    {
        VkDevice device = rhi.GetDevice();
        for(Batch& batch : mBatches)
        {
            for(StagingBlock& block : batch.Blocks)
                DestroyBlock(rhi, block);

            vkDestroyFence(device, batch.Fence, nullptr);
        }
        mBatches.clear();
        mSubmitOrder.clear();

        for(StagingBlock& block : mFreeBlocks)
            DestroyBlock(rhi, block);
        mFreeBlocks.clear();

        vkDestroyCommandPool(device, mCommandPool, nullptr); // Frees every batch command buffer
        mCommandPool = VK_NULL_HANDLE;
        mOpenBatch = -1;
        mInFlightStagingBytes = 0;
    }

    VulkanUploader::StagingAllocation VulkanUploader::AllocateStaging(const VulkanRHI& rhi, VkDeviceSize size)
    {
        // 16 covers every copy offset rule we hit: 4 byte buffer copies and 16 byte compressed blocks (BC/ASTC/ETC2)
        constexpr VkDeviceSize alignment = 16;

        Batch* batch = &OpenBatch(rhi);
        if(batch->StagingBytes > 0 && batch->StagingBytes + size > FLUSH_THRESHOLD)
        {
            Flush(rhi);
            batch = &OpenBatch(rhi);
        }

        StagingBlock* target = nullptr;
        for(StagingBlock& block : batch->Blocks)
        {
            const VkDeviceSize offset = (block.Used + alignment - 1) & ~(alignment - 1);
            if(offset + size <= block.Size)
            {
                block.Used = offset;
                target = &block;
                break;
            }
        }

        if(!target)
        {
            if(size <= BLOCK_SIZE && !mFreeBlocks.empty())
            {
                batch->Blocks.push_back(mFreeBlocks.back());
                mFreeBlocks.pop_back();
            }
            else
                batch->Blocks.push_back(CreateBlock(rhi, std::max(size, BLOCK_SIZE)));

            target = &batch->Blocks.back();
            target->Used = 0;
        }

        StagingAllocation result;
        result.Buffer = target->Buffer;
        result.Offset = target->Used;
        result.MappedData = target->MappedData + target->Used;
        target->Used += size;
        batch->StagingBytes += size;
        return result;
    }

    VkCommandBuffer VulkanUploader::GetCommandBuffer(const VulkanRHI& rhi)
    {
        return OpenBatch(rhi).Cmd;
    }

    void VulkanUploader::Flush(const VulkanRHI& rhi)
    {
        if(mOpenBatch < 0)
            return;

        SURGE_PROFILE_FUNC("VulkanUploader::Flush");
        Batch& batch = mBatches[mOpenBatch];

        // Make the transfer writes visible to every later consumer on this queue (vertex/index fetch, uniforms, shader reads)
        // Image layouts are already transitioned by the per-image barriers recorded with the copies
        VkMemoryBarrier memoryBarrier = {};
        memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        memoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        memoryBarrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(batch.Cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_VERTEX_INPUT_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                             0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);

        VK_CALL(vkEndCommandBuffer(batch.Cmd));

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &batch.Cmd;
        VK_CALL(vkQueueSubmit(rhi.GetQueue(), 1, &submitInfo, batch.Fence));

        batch.InFlight = true;
        mInFlightStagingBytes += batch.StagingBytes;
        mSubmitOrder.push_back(static_cast<Uint>(mOpenBatch));
        mOpenBatch = -1;

        // Bulk loading (hundreds of textures between two frames) must not grow staging memory without bound
        while(mInFlightStagingBytes > MAX_IN_FLIGHT_STAGING && !mSubmitOrder.empty())
            WaitForOldestBatch(rhi);
    }

    void VulkanUploader::Collect(const VulkanRHI& rhi)
    {
        VkDevice device = rhi.GetDevice();
        for(size_t i = 0; i < mSubmitOrder.size();)
        {
            Batch& batch = mBatches[mSubmitOrder[i]];
            if(vkGetFenceStatus(device, batch.Fence) == VK_SUCCESS)
            {
                RecycleBatch(rhi, batch);
                mSubmitOrder.erase(mSubmitOrder.begin() + i);
            }
            else
                i++;
        }
    }

    VulkanUploader::Batch& VulkanUploader::OpenBatch(const VulkanRHI& rhi)
    {
        if(mOpenBatch >= 0)
            return mBatches[mOpenBatch];

        int freeIndex = -1;
        for(size_t i = 0; i < mBatches.size(); i++)
        {
            if(!mBatches[i].InFlight)
            {
                freeIndex = static_cast<int>(i);
                break;
            }
        }

        if(freeIndex < 0)
        {
            Batch batch;

            VkCommandBufferAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.commandPool = mCommandPool;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandBufferCount = 1;
            VK_CALL(vkAllocateCommandBuffers(rhi.GetDevice(), &allocInfo, &batch.Cmd));

            VkFenceCreateInfo fenceInfo = {};
            fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            VK_CALL(vkCreateFence(rhi.GetDevice(), &fenceInfo, nullptr, &batch.Fence));

            mBatches.push_back(std::move(batch));
            freeIndex = static_cast<int>(mBatches.size() - 1);
        }

        Batch& batch = mBatches[freeIndex];
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        VK_CALL(vkBeginCommandBuffer(batch.Cmd, &beginInfo));

        mOpenBatch = freeIndex;
        return batch;
    }

    VulkanUploader::StagingBlock VulkanUploader::CreateBlock(const VulkanRHI& rhi, VkDeviceSize size) const
    {
        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
        allocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

        StagingBlock block;
        VmaAllocationInfo resultInfo = {};
        VK_CALL(vmaCreateBuffer(rhi.GetAllocator(), &bufferInfo, &allocInfo, &block.Buffer, &block.Allocation, &resultInfo));
        SG_ASSERT(resultInfo.pMappedData, "VulkanUploader: staging block failed to map");
        SET_VMA_DEBUG_NAME(rhi, block.Allocation, "UploadStaging");

        block.MappedData = static_cast<uint8_t*>(resultInfo.pMappedData);
        block.Size = size;
        block.Used = 0;
        return block;
    }

    void VulkanUploader::DestroyBlock(const VulkanRHI& rhi, StagingBlock& block) const
    {
        if(block.Buffer != VK_NULL_HANDLE)
            vmaDestroyBuffer(rhi.GetAllocator(), block.Buffer, block.Allocation);
        block = {};
    }

    void VulkanUploader::RecycleBatch(const VulkanRHI& rhi, Batch& batch)
    {
        for(StagingBlock& block : batch.Blocks)
        {
            if(block.Size == BLOCK_SIZE && mFreeBlocks.size() < MAX_FREE_BLOCKS)
            {
                block.Used = 0;
                mFreeBlocks.push_back(block);
            }
            else
                DestroyBlock(rhi, block);
        }
        batch.Blocks.clear();

        mInFlightStagingBytes -= std::min(mInFlightStagingBytes, batch.StagingBytes);
        batch.StagingBytes = 0;
        batch.InFlight = false;

        VK_CALL(vkResetFences(rhi.GetDevice(), 1, &batch.Fence));
        VK_CALL(vkResetCommandBuffer(batch.Cmd, 0));
    }

    void VulkanUploader::WaitForOldestBatch(const VulkanRHI& rhi)
    {
        SURGE_PROFILE_FUNC("VulkanUploader::WaitForOldestBatch");
        Batch& batch = mBatches[mSubmitOrder.front()];
        VK_CALL(vkWaitForFences(rhi.GetDevice(), 1, &batch.Fence, VK_TRUE, UINT64_MAX));
        RecycleBatch(rhi, batch);
        mSubmitOrder.erase(mSubmitOrder.begin());
    }
}
