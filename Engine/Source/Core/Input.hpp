// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once

#include "Core/KeyCodes.hpp"

namespace Surge
{
	class SURGE_API Input
	{
	public:
		static void Init();
		
		static bool GetKeyDown(KeyCode key);
		static bool GetKeyUp(KeyCode key);
	};
}