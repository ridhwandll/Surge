// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Export/Platforms/Android/AndroidDevice.hpp"
#include "Export/Platforms/Android/AndroidToolParsers.hpp"
#include "Export/ExportUtils.hpp"

#include <chrono>
#include <exception>
#include <format>

namespace Surge
{
    using ExportUtils::Quote;

    namespace
    {
        constexpr int DEVICE_WAIT_SECONDS = 30; // Long enough to unlock the phone and accept the USB debugging prompt

        // "adb devices": one "<serial>\t<state>" line per attached device (device, unauthorized, offline, ...). Nothing is logged, it is polled
        class DeviceListParser : public ToolOutputParser
        {
        public:
            struct Device
            {
                String Serial;
                String State;
            };

            virtual void Begin() override { Devices.clear(); }
            virtual void OnLine(ExportSession&, const String& line) override
            {
                const size_t tab = line.find('\t');
                if(tab != String::npos)
                    Devices.push_back({ line.substr(0, tab), line.substr(tab + 1) });
            }

            Vector<Device> Devices;
        };
    } // namespace

    AndroidDevice::InstallResult AndroidDevice::InstallAndLaunch(ExportSession& session, const Path& adbPath, const Path& apk, const String& packageId, bool launch, float begin, const String& serial)
    {
        String adb = Quote(adbPath.string());
        if(!serial.empty())
            adb += " -s " + Quote(serial);
        AdbOutputParser parser;

        session.SetStep("Installing on device", begin, 0.99f);
        session.SetDetail("adb install " + apk.filename().string());
        if(session.RunTool(adb + " install -r " + Quote(apk.string()), Path(), &parser) != 0)
            return session.IsCancelled() ? InstallResult::CANCELLED : InstallResult::INSTALL_FAILED;

        if(!launch || session.IsCancelled())
            return InstallResult::INSTALLED;

        session.SetStep("Launching on device", 0.99f, 1.0f);
        const String command = adb + " shell monkey -p " + packageId + " -c android.intent.category.LAUNCHER 1";
        if(session.RunTool(command, Path(), &parser) != 0)
            return session.IsCancelled() ? InstallResult::CANCELLED : InstallResult::LAUNCH_FAILED;
        return InstallResult::INSTALLED;
    }

    void AndroidDeviceInstaller::Start(Request request)
    {
        if(IsBusy())
            return;
        Join();

        mRequest = std::move(request);
        mSession.Reset();
        mSession.Start("Restarting adb");
        mSession.SetState(ExportState::RUNNING);
        mSession.Log(Severity::Info, "Reattaching adb to install " + mRequest.Apk.filename().string());

        mWorker = std::thread([this]() {
            bool ok = false;
            try
            {
                ok = Run();
            }
            catch(const std::exception& e)
            {
                mSession.Error(String("Unhandled exception: ") + e.what());
            }

            if(ok)
                mSession.Log(Severity::Info, "Installed on the device.");
            else if(!mSession.IsCancelled())
                mSession.Log(Severity::Error, "Installing on the device FAILED.");
            mSession.SetState(ok ? ExportState::SUCCEEDED : (mSession.IsCancelled() ? ExportState::CANCELLED : ExportState::FAILED)); // Last write
        });
    }

    void AndroidDeviceInstaller::Reset()
    {
        if(IsBusy())
            return;
        Join();
        mSession.Reset();
    }

    void AndroidDeviceInstaller::Shutdown()
    {
        mSession.RequestCancel();
        Join();
    }

    void AndroidDeviceInstaller::Join()
    {
        if(mWorker.joinable())
            mWorker.join();
    }

    bool AndroidDeviceInstaller::Run()
    {
        const String adb = Quote(mRequest.Adb.string());
        AdbOutputParser parser;

        // A stuck server (device shown as offline, or never listed after replugging) only recovers by restarting it
        mSession.SetStep("Restarting adb", 0.0f, 0.15f);
        mSession.RunTool(adb + " kill-server", Path(), &parser); // Fails when no server is running, that's fine
        if(mSession.IsCancelled())
            return false;
        if(mSession.RunTool(adb + " start-server", Path(), &parser) != 0)
        {
            if(!mSession.IsCancelled())
                mSession.Error("Could not start the adb server. Another program (an emulator or a phone suite) may be using port 5037.");
            return false;
        }

        mSession.SetStep("Waiting for the device", 0.15f, 0.5f);
        String serial;
        if(!WaitForDevice(serial))
            return false;

        switch(AndroidDevice::InstallAndLaunch(mSession, mRequest.Adb, mRequest.Apk, mRequest.PackageId, mRequest.Launch, 0.5f, serial))
        {
            case AndroidDevice::InstallResult::INSTALLED:
                return true;
            case AndroidDevice::InstallResult::INSTALL_FAILED:
                mSession.Error("adb install failed, see the Console");
                return false;
            case AndroidDevice::InstallResult::LAUNCH_FAILED:
                mSession.SetResultWarning("Installed, but launching the app on the device failed.");
                return true;
            case AndroidDevice::InstallResult::CANCELLED:
                return false;
        }
        return false;
    }

    bool AndroidDeviceInstaller::WaitForDevice(String& outSerial)
    {
        const String command = Quote(mRequest.Adb.string()) + " devices";
        String problem = "No device found. Connect it over USB and enable USB debugging (Settings > Developer options).";
        DeviceListParser parser;

        for(int second = 0; second < DEVICE_WAIT_SECONDS; ++second)
        {
            if(mSession.RunTool(command, Path(), &parser) != 0)
            {
                if(!mSession.IsCancelled())
                    mSession.Error("'adb devices' failed");
                return false;
            }

            for(const DeviceListParser::Device& device : parser.Devices)
            {
                if(device.State == "device")
                {
                    outSerial = device.Serial;
                    mSession.Log(Severity::Info, "Device attached: " + device.Serial);
                    return true;
                }
            }

            if(!parser.Devices.empty())
            {
                const String& state = parser.Devices.front().State;
                if(state == "unauthorized")
                    problem = "The device is unauthorized. Unlock it and accept the 'Allow USB debugging?' prompt.";
                else if(state == "offline")
                    problem = "The device is offline. Unplug the USB cable and plug it back in.";
                else
                    problem = std::format("The device is in the '{}' state.", state);
            }
            mSession.SetDetail(problem);
            mSession.SetStepFraction(static_cast<float>(second + 1) / DEVICE_WAIT_SECONDS);

            for(int i = 0; i < 10 && !mSession.IsCancelled(); ++i)
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            if(mSession.IsCancelled())
                return false;
        }

        mSession.Error(problem);
        return false;
    }

} // namespace Surge
