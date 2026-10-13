// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#ifdef SURGE_PLATFORM_ANDROID
#include <game-activity/native_app_glue/android_native_app_glue.h>

namespace Surge::Android
{
    // Set by android_main() before Core::Initialize() is called
    // Provides access to the android_app context for platform and window code
    extern android_app* GAndroidApp;

    // Forwarded from AndroidWindow::HandleAppCmd to the text input state in AndroidPlatform.cpp
    void OnSoftKeyboardVisibilityChanged(bool visible);
    void OnEditorAction(int action);

} // namespace Surge::Android

#endif // SURGE_PLATFORM_ANDROID