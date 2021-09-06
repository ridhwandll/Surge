// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Memory.hpp"
#include "Surge/Graphics/Shader.hpp"

namespace Surge
{
	struct VertexBufferLayout {}; // TODO: This is not made??

	enum class PrimitiveType
	{
		Points,
		Lines,
		LineStrip,
		Triangles,
		TriangleStrip
	};

	struct GraphicsPipelineSpecification
	{
		Ref<Shader> Shader;
		VertexBufferLayout VertexBufferLayout;
		PrimitiveType Topology = PrimitiveType::Triangles;
		float LineWidth = 1.0f;
		bool UseDepth = true;
		bool UseStencil = false;
	};

	class GraphicsPipeline : public RefCounted
	{
	public:
		GraphicsPipeline() = default;
		virtual ~GraphicsPipeline() {}

		virtual const GraphicsPipelineSpecification& GetPipelineSpecification() const = 0;
		virtual void Bind() = 0;

		static Ref<GraphicsPipeline> Create(const GraphicsPipelineSpecification& pipelineSpec);
	};


}