// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Defines.hpp"
#include "Surge/Core/Path.hpp"
#include "Surge/Core/String.hpp"
#include "Surge/Core/Vector.hpp"
#include "Surge/Graphics/RHI/RHIHandle.hpp"
#include <imgui.h>

#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>
#include <unordered_map>

namespace Surge
{
    // Content Browser previews of image files, decoded from the source file on a worker thread (no cooking, works for unimported files too),
    // uploaded a few per frame and evicted least recently used first. Editor only
    class ThumbnailCache
    {
    public:
        static constexpr Uint THUMBNAIL_SIZE = 128; // Longest side, aspect ratio is kept
        static constexpr size_t MAX_ENTRIES = 256;
        static constexpr Uint UPLOADS_PER_FRAME = 4;

        struct Thumbnail
        {
            ImTextureID TextureID = 0;
            float AspectRatio = 1.0f; // Width / height
        };

        ThumbnailCache() = default;
        ~ThumbnailCache() { Shutdown(); }
        SURGE_DISABLE_COPY_AND_MOVE(ThumbnailCache);

        void Initialize(SamplerHandle sampler);
        void Shutdown(); // Joins the worker and frees every image, safe to call twice

        // Null TextureID until the image was decoded, cheap enough to call for every visible item each frame. writeTime invalidates stale entries
        Thumbnail Get(const Path& imageFile, std::filesystem::file_time_type writeTime);

        void Update(); // Main thread, once per frame: uploads finished thumbnails and evicts old ones
        size_t GetPendingCount() const { return mPendingCount; } // Queued or waiting for upload, as of the last Update()

    private:
        struct Entry
        {
            ImageHandle Image;
            Thumbnail Value;
            std::filesystem::file_time_type WriteTime;
            uint64_t LastUsedFrame = 0;
            bool Pending = true;
            bool Failed = false;
        };

        struct Job
        {
            String Key;
            Path File;
            std::filesystem::file_time_type WriteTime;
        };

        struct Decoded
        {
            String Key;
            std::filesystem::file_time_type WriteTime;
            Vector<Byte> Pixels; // RGBA8, empty when decoding failed
            Uint Width = 0;
            Uint Height = 0;
        };

        void WorkerLoop();
        static Decoded Decode(const Job& job);
        void Evict();

    private:
        SamplerHandle mSampler;
        std::unordered_map<String, Entry> mEntries;
        uint64_t mFrame = 0;
        size_t mPendingCount = 0;

        std::thread mWorker;
        std::mutex mMutex;
        std::condition_variable mWakeUp;
        std::deque<Job> mJobs;
        Vector<Decoded> mFinished;
        bool mStopWorker = false;
        bool mInitialized = false;
    };

} // namespace Surge
