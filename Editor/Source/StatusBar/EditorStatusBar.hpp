// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Defines.hpp"
#include "Surge/Core/String.hpp"
#include "Surge/Core/Vector.hpp"
#include <chrono>
#include <mutex>

namespace Surge
{
    // Footer locked to the bottom of the editor window: the latest log line or a notification on the left,
    // running tasks with progress bars on the right. Notify() and the task calls are thread-safe, Render() is main thread only
    class EditorStatusBar
    {
    public:
        static constexpr float ROW_HEIGHT = 26.0f;
        static constexpr float BOTTOM_PADDING = 6.0f;
        static constexpr float HEIGHT = ROW_HEIGHT + BOTTOM_PADDING;
        static constexpr float INDETERMINATE = -1.0f;

        static EditorStatusBar& Get();

        // Shown instead of the latest log line for a few seconds
        void Notify(Severity severity, const String& text, float seconds = 5.0f);

        // A task is identified by its owner's key. Call SetTask() whenever it advances (progress: 0..1 or INDETERMINATE), EndTask() when done
        void SetTask(const String& key, const String& title, float progress = INDETERMINATE, const String& detail = {});
        void EndTask(const String& key);

        void Render(); // Main thread, inside the ImGui frame

    private:
        using TimePoint = std::chrono::steady_clock::time_point;

        struct Task
        {
            String Key;
            String Title;
            String Detail;
            float Progress = INDETERMINATE;
            float DisplayProgress = 0.0f; // Eases towards Progress, main thread
            TimePoint StartTime;
        };

        struct Notification
        {
            Severity Level = Severity::Info;
            String Text;
            float Seconds = 0.0f;
            TimePoint Time;
        };

        void SyncLatestLog();
        void DrawMessage(float x, float width, float height);
        float DrawTasks(float rightX, float height); // Right aligned at rightX, returns the width it took

    private:
        std::mutex mMutex; // Guards mTasks and mNotification
        Vector<Task> mTasks;
        Notification mNotification;

        // Main thread: copies taken under the lock, drawn without it
        Vector<Task> mDrawTasks; // Only the ones past the show delay, oldest first
        Notification mDrawNotification;

        // Main thread: the console's newest message, refreshed when its revision changes
        uint64_t mConsoleRevision = UINT64_MAX;
        bool mHasLog = false;
        Severity mLogLevel = Severity::Info;
        String mLogText;
        char mLogTime[9] = {};
        Uint mLogCount = 1;
    };

} // namespace Surge
