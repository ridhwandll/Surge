// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once

namespace Surge
{
    class SURGE_API Window
    {
    public:
        static Window* Create(int width, int height, const String& title);
        virtual ~Window() = default;

        virtual bool IsOpen() const = 0;
        virtual void Update() = 0;

        int GetWidth() const { return mWidth; }
        int GetHeight() const { return mHeight; }
        const String& GetTitle() const { return mTitle; }
    protected:
        int mWidth = 0;
        int mHeight = 0;
        String mTitle;
    };
}