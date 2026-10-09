// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Asset/ThumbnailCache.hpp"
#include "Surge/Core/Core.hpp"
#include "Surge/Graphics/Renderer/Renderer.hpp"

#include <stb_image.h>
#include <algorithm>
#include <cmath>
#include <fstream>

namespace Surge
{
    void ThumbnailCache::Initialize(SamplerHandle sampler)
    {
        mSampler = sampler;
        mStopWorker = false;
        mWorker = std::thread([this]() { WorkerLoop(); });
        mInitialized = true;
    }

    void ThumbnailCache::Shutdown()
    {
        if(!mInitialized)
            return;

        {
            std::lock_guard<std::mutex> lock(mMutex);
            mStopWorker = true;
            mJobs.clear();
        }
        mWakeUp.notify_all();
        mWorker.join();
        mFinished.clear();

        Scope<GraphicsRHI>& rhi = Core::GetRenderer()->GetRHI();
        for(auto& [key, entry] : mEntries)
        {
            if(!entry.Image.IsNull())
                rhi->DestroyImage(entry.Image);
        }
        mEntries.clear();
        mInitialized = false;
    }

    ThumbnailCache::Thumbnail ThumbnailCache::Get(const Path& imageFile, std::filesystem::file_time_type writeTime)
    {
        if(!mInitialized)
            return {};

        String key = imageFile.generic_string();
        auto it = mEntries.find(key);
        if(it != mEntries.end())
        {
            Entry& entry = it->second;
            entry.LastUsedFrame = mFrame;
            if(entry.WriteTime != writeTime && !entry.Pending)
            {
                // The file changed: decode again, the old preview stays up until the new one is ready
                entry.WriteTime = writeTime;
                entry.Pending = true;
                entry.Failed = false;
                {
                    std::lock_guard<std::mutex> lock(mMutex);
                    mJobs.push_back({ key, imageFile, writeTime });
                }
                mWakeUp.notify_one();
            }
            return entry.Value;
        }

        Entry& entry = mEntries[key];
        entry.WriteTime = writeTime;
        entry.LastUsedFrame = mFrame;
        {
            std::lock_guard<std::mutex> lock(mMutex);
            mJobs.push_back({ std::move(key), imageFile, writeTime });
        }
        mWakeUp.notify_one();
        return {};
    }

    void ThumbnailCache::Update()
    {
        if(!mInitialized)
            return;

        mFrame++;

        Vector<Decoded> finished;
        {
            std::lock_guard<std::mutex> lock(mMutex);
            const size_t count = std::min<size_t>(mFinished.size(), UPLOADS_PER_FRAME);
            finished.reserve(count);
            for(size_t i = 0; i < count; i++)
                finished.push_back(std::move(mFinished[i]));
            mFinished.erase(mFinished.begin(), mFinished.begin() + static_cast<std::ptrdiff_t>(count));
            mPendingCount = mJobs.size() + mFinished.size();
        }

        Scope<GraphicsRHI>& rhi = Core::GetRenderer()->GetRHI();
        for(Decoded& decoded : finished)
        {
            auto it = mEntries.find(decoded.Key);
            if(it == mEntries.end() || it->second.WriteTime != decoded.WriteTime)
                continue; // Evicted meanwhile, or a newer version is on its way

            Entry& entry = it->second;
            entry.Pending = false;
            if(decoded.Pixels.empty())
            {
                entry.Failed = true;
                continue;
            }

            ImageDesc desc = {};
            desc.Width = decoded.Width;
            desc.Height = decoded.Height;
            desc.MipLevel = static_cast<Uint>(std::floor(std::log2(std::max(decoded.Width, decoded.Height)))) + 1; // Drawn smaller than the source
            desc.Format = ImageFormat::RGBA8_UNORM;
            desc.Usage = ImageUsage::SAMPLED | ImageUsage::TRANSFER_DST;
            desc.Sampler = mSampler;
            desc.InitialData = decoded.Pixels.data();
            desc.DataSize = static_cast<Uint>(decoded.Pixels.size());
            desc.GenerateImGuiID = true;
            desc.DebugName = "ContentBrowserThumbnail";

            if(!entry.Image.IsNull())
                rhi->DestroyImage(entry.Image);
            entry.Image = rhi->CreateImage(desc);
            entry.Value.TextureID = rhi->GetImGuiImage(entry.Image);
            entry.Value.AspectRatio = static_cast<float>(decoded.Width) / static_cast<float>(decoded.Height);
        }

        if(mEntries.size() > MAX_ENTRIES)
            Evict();
    }

    void ThumbnailCache::Evict()
    {
        // Least recently drawn first, down to 90% so this doesn't run every frame
        Vector<Pair<uint64_t, const String*>> byAge;
        byAge.reserve(mEntries.size());
        for(const auto& [key, entry] : mEntries)
        {
            if(entry.LastUsedFrame + 1 < mFrame) // Never what is on screen right now
                byAge.push_back({ entry.LastUsedFrame, &key });
        }
        std::sort(byAge.begin(), byAge.end(), [](const auto& a, const auto& b) { return a.Data1 < b.Data1; });

        const size_t target = MAX_ENTRIES * 9 / 10;
        Vector<String> toRemove;
        for(const auto& [frame, key] : byAge)
        {
            if(mEntries.size() - toRemove.size() <= target)
                break;
            toRemove.push_back(*key);
        }

        Scope<GraphicsRHI>& rhi = Core::GetRenderer()->GetRHI();
        for(const String& key : toRemove)
        {
            auto it = mEntries.find(key);
            if(!it->second.Image.IsNull())
                rhi->DestroyImage(it->second.Image); // Deferred until the GPU is done with it
            mEntries.erase(it);
        }
    }

    void ThumbnailCache::WorkerLoop()
    {
        while(true)
        {
            Job job;
            {
                std::unique_lock<std::mutex> lock(mMutex);
                mWakeUp.wait(lock, [this]() { return mStopWorker || !mJobs.empty(); });
                if(mStopWorker)
                    return;

                // Newest request first: that is what the user is looking at
                job = std::move(mJobs.back());
                mJobs.pop_back();
            }

            Decoded decoded = Decode(job);
            {
                std::lock_guard<std::mutex> lock(mMutex);
                mFinished.push_back(std::move(decoded));
            }
        }
    }

    ThumbnailCache::Decoded ThumbnailCache::Decode(const Job& job)
    {
        Decoded result;
        result.Key = job.Key;
        result.WriteTime = job.WriteTime;

        // No engine file helpers here, they log and this runs off the main thread
        std::ifstream file(job.File, std::ios::binary | std::ios::ate);
        if(!file)
            return result;
        const std::streamsize fileSize = file.tellg();
        if(fileSize <= 0)
            return result;
        Vector<Byte> fileData(static_cast<size_t>(fileSize));
        file.seekg(0);
        file.read(reinterpret_cast<char*>(fileData.data()), fileSize);

        int width = 0, height = 0, channels = 0;
        stbi_uc* source = stbi_load_from_memory(fileData.data(), static_cast<int>(fileData.size()), &width, &height, &channels, 4);
        if(!source || width <= 0 || height <= 0)
        {
            stbi_image_free(source);
            return result;
        }

        const float scale = std::min(1.0f, static_cast<float>(THUMBNAIL_SIZE) / static_cast<float>(std::max(width, height)));
        const int targetWidth = std::max(1, static_cast<int>(std::lround(width * scale)));
        const int targetHeight = std::max(1, static_cast<int>(std::lround(height * scale)));
        result.Width = static_cast<Uint>(targetWidth);
        result.Height = static_cast<Uint>(targetHeight);
        result.Pixels.resize(static_cast<size_t>(targetWidth) * targetHeight * 4);

        // Box filter with premultiplied alpha, so transparent texels don't darken the edges
        for(int y = 0; y < targetHeight; y++)
        {
            const int y0 = y * height / targetHeight;
            const int y1 = std::max(y0 + 1, (y + 1) * height / targetHeight);
            for(int x = 0; x < targetWidth; x++)
            {
                const int x0 = x * width / targetWidth;
                const int x1 = std::max(x0 + 1, (x + 1) * width / targetWidth);

                uint64_t r = 0, g = 0, b = 0, a = 0;
                for(int sy = y0; sy < y1; sy++)
                {
                    const stbi_uc* texel = source + (static_cast<size_t>(sy) * width + x0) * 4;
                    for(int sx = x0; sx < x1; sx++, texel += 4)
                    {
                        r += static_cast<uint64_t>(texel[0]) * texel[3];
                        g += static_cast<uint64_t>(texel[1]) * texel[3];
                        b += static_cast<uint64_t>(texel[2]) * texel[3];
                        a += texel[3];
                    }
                }

                const uint64_t count = static_cast<uint64_t>(x1 - x0) * (y1 - y0);
                Byte* out = result.Pixels.data() + (static_cast<size_t>(y) * targetWidth + x) * 4;
                out[0] = a ? static_cast<Byte>(r / a) : 0;
                out[1] = a ? static_cast<Byte>(g / a) : 0;
                out[2] = a ? static_cast<Byte>(b / a) : 0;
                out[3] = static_cast<Byte>(a / count);
            }
        }

        stbi_image_free(source);
        return result;
    }

} // namespace Surge
