// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Pch.hpp"
#include "Device.h"

#ifdef SURGE_WINDOWS
#include "Vulkan/VulkanDevice.hpp"
#endif

namespace Surge
{
	Ref<Device> Device::Create(bool requestRaytracing)
	{
#ifdef SURGE_WINDOWS
		return CreateRef<VulkanDevice>(requestRaytracing);
#endif
		return nullptr;
	}
}