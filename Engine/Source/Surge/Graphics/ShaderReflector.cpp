// Copyright (c) - SurgeTechnologies - All rights reserved
#include "ShaderReflector.hpp"
#include <SPIRV-Cross/spirv_glsl.hpp>

namespace Surge
{
    namespace Utils
    {
        ShaderDataType SPVTypeToShaderDataType(const spirv_cross::SPIRType& spvType)
        {
            if (spvType.basetype == spirv_cross::SPIRType::Float && spvType.vecsize == 1 && spvType.columns == 1)
                return ShaderDataType::Float;
            if (spvType.basetype == spirv_cross::SPIRType::Float && spvType.vecsize == 2 && spvType.columns == 1)
                return ShaderDataType::Float2;
            if (spvType.basetype == spirv_cross::SPIRType::Float && spvType.vecsize == 3 && spvType.columns == 1)
                return ShaderDataType::Float3;
            if (spvType.basetype == spirv_cross::SPIRType::Float && spvType.vecsize == 4 && spvType.columns == 1)
                return ShaderDataType::Float4;
            if (spvType.basetype == spirv_cross::SPIRType::Float && spvType.vecsize == 2 && spvType.columns == 2)
                return ShaderDataType::Mat2;
            if (spvType.basetype == spirv_cross::SPIRType::Float && spvType.vecsize == 3 && spvType.columns == 3)
                return ShaderDataType::Mat3;
            if (spvType.basetype == spirv_cross::SPIRType::Float && spvType.vecsize == 4 && spvType.columns == 4)
                return ShaderDataType::Mat4;
            if (spvType.basetype == spirv_cross::SPIRType::UInt && spvType.vecsize == 1 && spvType.columns == 1)
                return ShaderDataType::UInt;
            if (spvType.basetype == spirv_cross::SPIRType::Int && spvType.vecsize == 1 && spvType.columns == 1)
                return ShaderDataType::Int;
            if (spvType.basetype == spirv_cross::SPIRType::Boolean && spvType.vecsize == 1 && spvType.columns == 1)
                return ShaderDataType::Bool;

            SG_ASSERT_INTERNAL("No spirv_cross::SPIRType matches Surge::ShaderDataType!");
            return ShaderDataType::None;
        }
    };

    ShaderReflectionData ShaderReflector::Reflect(const SPIRVHandle& spirvHandle)
    {
        spirv_cross::Compiler compiler(spirvHandle.SPIRV);
        spirv_cross::ShaderResources resources = compiler.get_shader_resources();

        ShaderReflectionData result;
        result.SetDomain(spirvHandle.Type);

        // Fetch the textures
        for (const spirv_cross::Resource& resource : resources.separate_images)
        {
            ShaderResource res;
            res.Binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
            res.Set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
            res.Name = resource.name;
            result.PushResource(res);
        }

        // Fetch the StageInputs
        Uint elementOffset = 0;
        for (const spirv_cross::Resource& resource : resources.stage_inputs)
        {
            ShaderStageInput stageInput;

            const spirv_cross::SPIRType& spvType = compiler.get_type(resource.base_type_id);
            stageInput.Location = compiler.get_decoration(resource.id, spv::DecorationLocation);
            stageInput.Name = resource.name;
            stageInput.DataType = Utils::SPVTypeToShaderDataType(spvType);
            stageInput.Size = ShaderDataTypeSize(stageInput.DataType);
            stageInput.Offset = elementOffset;

            result.PushStageInput(stageInput);
            elementOffset += stageInput.Size;
        }

        // Fetch all the Uniform/Constannt buffers
        for (const spirv_cross::Resource& resource : resources.uniform_buffers)
        {
            ShaderBuffer buffer;
            const spirv_cross::SPIRType& bufferType = compiler.get_type(resource.base_type_id);

            buffer.Size = static_cast<Uint>(compiler.get_declared_struct_size(bufferType));
            buffer.Set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
            buffer.Binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
            buffer.BufferName = resource.name;

            for (Uint i = 0; i < bufferType.member_types.size(); i++)
            {
                ShaderBufferMember bufferMember;
                bufferMember.Name = buffer.BufferName + '.' + compiler.get_member_name(bufferType.self, i);
                bufferMember.MemoryOffset = compiler.type_struct_member_offset(bufferType, i); // In bytes
                buffer.Members.emplace_back(bufferMember);
            }

            result.PushBuffer(buffer);
        }
        return result;
    }
}
