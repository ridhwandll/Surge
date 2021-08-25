// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Pch.hpp"
#include "Renderer.hpp"

namespace Surge
{
	RendererData Renderer::sData;

	void Renderer::Init()
	{
		sData.device = Device::Create(true);
	}

	void Renderer::Shutdown()
	{
		sData.device.reset();
	}
}