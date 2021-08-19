// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once

namespace Surge
{
	class SURGE_API Window
	{
	public:
		static Window* Create(int width, int height, const std::string& title);
		virtual ~Window() = default;

		virtual bool IsOpen() = 0;
		virtual void Update() = 0;

		int GetWidth() { return mWidth; }
		int GetHeight() { return mHeight; }
		const std::string& GetTitle() const { return mTitle; }
	protected:
		int mWidth = 0;
		int mHeight = 0;
		std::string mTitle;
	};
}