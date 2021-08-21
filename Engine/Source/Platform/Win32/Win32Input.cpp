// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Pch.hpp"
#include "Core/Input.hpp"

#include <Windows.h>

namespace Surge
{
	static unsigned char sKeyMaps[256];

	void Input::Init()
	{
		for (int i = 0; i < 256; i++)
			sKeyMaps[i] = false;
	}

	bool Input::GetKeyDown(KeyCode key)
	{
		if (GetKeyboardState(sKeyMaps))
		{
			if (sKeyMaps[key] & 0x80)
				return true;
		}

		return false;
	}

	bool Input::GetKeyUp(KeyCode key)
	{
		return !Input::GetKeyDown(key);
	}
}