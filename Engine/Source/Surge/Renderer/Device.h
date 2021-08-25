// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once

namespace Surge
{
	class SURGE_API Device
	{
	public:
		Device() = default;
		virtual ~Device() = default;

		static Ref<Device> Create(bool requestRaytracing);
	};
}