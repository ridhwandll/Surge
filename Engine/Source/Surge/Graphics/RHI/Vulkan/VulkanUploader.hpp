// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Defines.hpp"
#include "Surge/Core/Vector.hpp"
#include <volk.h>
#include <vk_mem_alloc.h>

namespace Surge
{
    class VulkanRHI;

    // Asynchronous resource uploads
    // Buffer/image uploads are recorded into an upload batch (own command buffer + staging blocks) instead of a blocking one-time
    // submit + vkQueueWaitIdle. The open batch is submitted right before the next frame submission (same queue, so submission
    // order guarantees the copies land before any use) or earlier if it grows too large. Staging memory is recycled once the
    // batch fence signals, the CPU never waits for the GPU unless the in-flight staging budget is exceeded
    class VulkanUploader
    {
    public:
        struct StagingAllocation
        {
            VkBuffer Buffer = VK_NULL_HANDLE;
            VkDeviceSize Offset = 0;
            void* MappedData = nullptr;
        };

    public:
        void Initialize(const VulkanRHI& rhi);
        void Shutdown(const VulkanRHI& rhi); // Device must be idle

        // Reserves host visible staging memory inside the open batch, the returned memory is valid until the batch completes
        StagingAllocation AllocateStaging(const VulkanRHI& rhi, VkDeviceSize size);

        // Command buffer of the open batch, every copy/barrier for an upload goes here
        VkCommandBuffer GetCommandBuffer(const VulkanRHI& rhi);

        // Submits the open batch (if any) without waiting
        void Flush(const VulkanRHI& rhi);

        // Recycles batches whose fence has signaled, non-blocking
        void Collect(const VulkanRHI& rhi);

        bool HasOpenBatch() const { return mOpenBatch >= 0; }

    private:
        struct StagingBlock
        {
            VkBuffer Buffer = VK_NULL_HANDLE;
            VmaAllocation Allocation = VK_NULL_HANDLE;
            uint8_t* MappedData = nullptr;
            VkDeviceSize Size = 0;
            VkDeviceSize Used = 0;
        };

        struct Batch
        {
            VkCommandBuffer Cmd = VK_NULL_HANDLE;
            VkFence Fence = VK_NULL_HANDLE;
            Vector<StagingBlock> Blocks;
            VkDeviceSize StagingBytes = 0;
            bool InFlight = false;
        };

        Batch& OpenBatch(const VulkanRHI& rhi);
        StagingBlock CreateBlock(const VulkanRHI& rhi, VkDeviceSize size) const;
        void DestroyBlock(const VulkanRHI& rhi, StagingBlock& block) const;
        void RecycleBatch(const VulkanRHI& rhi, Batch& batch);
        void WaitForOldestBatch(const VulkanRHI& rhi);

    private:
        static constexpr VkDeviceSize BLOCK_SIZE = 4 * 1024 * 1024;               // Default staging block, larger uploads get a dedicated block
        static constexpr VkDeviceSize FLUSH_THRESHOLD = 32 * 1024 * 1024;         // Submit the open batch early once it holds this much staging
        static constexpr VkDeviceSize MAX_IN_FLIGHT_STAGING = 128 * 1024 * 1024;  // Above this the CPU waits for the oldest batch (bulk loading)
        static constexpr Uint MAX_FREE_BLOCKS = 4;                                // Default sized blocks kept around for reuse (16 MB)

        VkCommandPool mCommandPool = VK_NULL_HANDLE;
        Vector<Batch> mBatches;
        Vector<StagingBlock> mFreeBlocks;
        Vector<Uint> mSubmitOrder; // Indices of in-flight batches, oldest first
        int mOpenBatch = -1;
        VkDeviceSize mInFlightStagingBytes = 0;
    };
}
