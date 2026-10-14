// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Export/ExportSession.hpp"
#include <thread>

namespace Surge
{
    namespace AndroidDevice
    {
        enum class InstallResult
        {
            INSTALLED = 0,
            INSTALL_FAILED,
            LAUNCH_FAILED,
            CANCELLED
        };

        // adb [-s serial] install -r <apk>, then starts the app's launcher activity when launch is set. Worker thread
        InstallResult InstallAndLaunch(ExportSession& session, const Path& adb, const Path& apk, const String& packageId, bool launch, float begin, const String& serial = {});
    } // namespace AndroidDevice

    // The retry after a failed 'Install on device': restarts the adb server, waits for an authorized device and installs the APK again.
    // Own worker and session, so the export result stays on screen. The public API is main thread only
    class AndroidDeviceInstaller
    {
    public:
        struct Request
        {
            Path Adb;
            Path Apk;
            String PackageId;
            bool Launch = true;
        };

        AndroidDeviceInstaller() = default;
        ~AndroidDeviceInstaller() { Shutdown(); }
        SURGE_DISABLE_COPY_AND_MOVE(AndroidDeviceInstaller);

        void Start(Request request);
        void Cancel() { mSession.RequestCancel(); }
        void Reset();    // Forgets the last result
        void Shutdown(); // Cancels and waits for the worker

        bool IsBusy() const { return IsExportBusy(mSession.GetState()); }
        ExportStatus GetStatus() const { return mSession.GetStatus(); }
        const Path& GetApk() const { return mRequest.Apk; }

    private:
        bool Run(); // Worker
        bool WaitForDevice(String& outSerial);
        void Join();

    private:
        ExportSession mSession;
        std::thread mWorker;
        Request mRequest;
    };

} // namespace Surge
