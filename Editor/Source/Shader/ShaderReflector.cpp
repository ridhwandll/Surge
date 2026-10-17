// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Shader/ShaderReflector.hpp"
#include "Surge/Graphics/RHI/Vulkan/VulkanUtils.hpp"
#include <SPIRV-Cross/spirv_glsl.hpp>

namespace Surge
{
    static ShaderDataType SPVTypeToShaderDataType(const spirv_cross::SPIRType& spvType)
    {
        if (spvType.basetype == spirv_cross::SPIRType::Float && spvType.vecsize == 1 && spvType.columns == 1)
            return ShaderDataType::FLOAT;
        if (spvType.basetype == spirv_cross::SPIRType::Float && spvType.vecsize == 2 && spvType.columns == 1)
            return ShaderDataType::FLOAT2;
        if (spvType.basetype == spirv_cross::SPIRType::Float && spvType.vecsize == 3 && spvType.columns == 1)
            return ShaderDataType::FLOAT3;
        if (spvType.basetype == spirv_cross::SPIRType::Float && spvType.vecsize == 4 && spvType.columns == 1)
            return ShaderDataType::FLOAT4;
        if (spvType.basetype == spirv_cross::SPIRType::Float && spvType.vecsize == 2 && spvType.columns == 2)
            return ShaderDataType::MAT2;
        if (spvType.basetype == spirv_cross::SPIRType::Float && spvType.vecsize == 3 && spvType.columns == 3)
            return ShaderDataType::MAT3;
        if (spvType.basetype == spirv_cross::SPIRType::Float && spvType.vecsize == 4 && spvType.columns == 4)
            return ShaderDataType::MAT4;
        if (spvType.basetype == spirv_cross::SPIRType::UInt && spvType.vecsize == 1 && spvType.columns == 1)
            return ShaderDataType::UINT;
        if (spvType.basetype == spirv_cross::SPIRType::Int && spvType.vecsize == 1 && spvType.columns == 1)
            return ShaderDataType::INT;
        if (spvType.basetype == spirv_cross::SPIRType::Boolean && spvType.vecsize == 1 && spvType.columns == 1)
            return ShaderDataType::BOOL;
        if (spvType.basetype == spirv_cross::SPIRType::Struct)
            return ShaderDataType::STRUCT;

        SG_ASSERT_INTERNAL("No spirv_cross::SPIRType matches Surge::ShaderDataType!");
        return ShaderDataType::NONE;
    }

    ////
    // ShaderReflector
    ////

    ShaderReflectionData ShaderReflector::Reflect(const String& shaderName, const Vector<SPIRVHandle>& spirvHandles)
    {
        ShaderReflectionData result;
        result.SetShaderName(shaderName);

        for (auto& handle : spirvHandles)
        {
            spirv_cross::Compiler compiler(handle.SPIRV);
            spirv_cross::ShaderResources resources = compiler.get_shader_resources();

            // Fetch the sampled textures
            for (const spirv_cross::Resource& resource : resources.sampled_images)
            {
                const spirv_cross::SPIRType& type = compiler.get_type(resource.type_id);
                uint32_t arrayCount = 1;
                if (!type.array.empty())
                    arrayCount = type.array[0];
                
                ShaderResource res;
                res.Binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
                res.Set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
                res.Name = resource.name;
                res.ShaderStages |= handle.Type;
                res.ShaderUsage = ShaderResource::Usage::SAMPLED;
                res.ArraySize = arrayCount;
                result.PushResource(res);
            }

            // Fetch the storage textures
            for (const spirv_cross::Resource& resource : resources.storage_images)
            {
                const spirv_cross::SPIRType& type = compiler.get_type(resource.type_id);
                uint32_t arrayCount = 1;
                if (!type.array.empty())
                    arrayCount = type.array[0];

                ShaderResource res;
                res.Binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
                res.Set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
                res.Name = resource.name;
                res.ShaderStages |= handle.Type;
                res.ShaderUsage = ShaderResource::Usage::STORAGE;
                res.ArraySize = arrayCount;
                result.PushResource(res);
            }

            // Fetch all the Uniform/Constant buffers
            for (const spirv_cross::Resource& resource : resources.uniform_buffers)
            {
                ShaderBuffer buffer;
                const spirv_cross::SPIRType& bufferType = compiler.get_type(resource.base_type_id);

                buffer.Size = static_cast<Uint>(compiler.get_declared_struct_size(bufferType));
                buffer.Set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
                buffer.Binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
                buffer.BufferName = resource.name;
                buffer.ShaderStages |= handle.Type;
                buffer.ShaderUsage = ShaderBuffer::Usage::UNIFORM;

                for (Uint i = 0; i < bufferType.member_types.size(); i++)
                {
                    const spirv_cross::SPIRType& spvType = compiler.get_type(bufferType.member_types[i]);

                    ShaderBufferMember bufferMember;
                    bufferMember.Name = compiler.get_member_name(bufferType.self, i);
                    bufferMember.MemoryOffset = compiler.type_struct_member_offset(bufferType, i); // In bytes
                    bufferMember.DataType = SPVTypeToShaderDataType(spvType);
                    bufferMember.Size = VulkanUtils::ShaderDataTypeSize(bufferMember.DataType);
                    buffer.Members.emplace_back(bufferMember);
                }
                result.PushBuffer(buffer);
            }

            // Fetch all the Storage buffers
            for (const spirv_cross::Resource& resource : resources.storage_buffers)
            {
                ShaderBuffer buffer;
                const spirv_cross::SPIRType& bufferType = compiler.get_type(resource.base_type_id);

                buffer.Size = static_cast<Uint>(compiler.get_declared_struct_size(bufferType));
                buffer.Set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
                buffer.Binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
                buffer.BufferName = resource.name;
                buffer.ShaderStages |= handle.Type;
                buffer.ShaderUsage = ShaderBuffer::Usage::STORAGE;

                for (Uint i = 0; i < bufferType.member_types.size(); i++)
                {
                    const spirv_cross::SPIRType& spvType = compiler.get_type(bufferType.member_types[i]);

                    ShaderBufferMember bufferMember;
                    bufferMember.Name = compiler.get_member_name(bufferType.self, i);
                    bufferMember.MemoryOffset = compiler.type_struct_member_offset(bufferType, i); // In bytes
                    bufferMember.DataType = SPVTypeToShaderDataType(spvType);
                    bufferMember.Size = VulkanUtils::ShaderDataTypeSize(bufferMember.DataType);
                    buffer.Members.emplace_back(bufferMember);
                }

                result.PushBuffer(buffer);
            }

            // Fetch the StageInputs
            for (const spirv_cross::Resource& resource : resources.stage_inputs)
            {
                ShaderStageInput stageInput;

                const spirv_cross::SPIRType& spvType = compiler.get_type(resource.base_type_id);
                Uint location = compiler.get_decoration(resource.id, spv::DecorationLocation);
                stageInput.Name = resource.name;
                stageInput.DataType = SPVTypeToShaderDataType(spvType);
                stageInput.Size = stageInput.DataType == ShaderDataType::STRUCT ? 0 : VulkanUtils::ShaderDataTypeSize(stageInput.DataType);
                stageInput.Offset = 0; // temporary, calculated later

                result.PushStageInput(stageInput, handle.Type, location);
            }

            // Calculating the offsets after the locations are sorted

            Uint elementOffset = 0;
            auto itr = result.mStageInputs.find(handle.Type);
            if (itr != result.mStageInputs.end())
            {
                for (auto& [location, stageInput] : result.mStageInputs.at(handle.Type))
                {
                    stageInput.Offset = elementOffset;
                    elementOffset += stageInput.Size;
                }
            }

            // Fetch Push Constants
            for (const spirv_cross::Resource& resource : resources.push_constant_buffers)
            {
                ShaderPushConstant pushConstant;
                const spirv_cross::SPIRType& bufferType = compiler.get_type(resource.base_type_id);

                pushConstant.BufferName = resource.name;
                pushConstant.Size = static_cast<Uint>(compiler.get_declared_struct_size(bufferType));
                pushConstant.ShaderStages |= handle.Type;
                result.PushBufferPushConstant(pushConstant);
            }

            // Cross compilation for example, we need them for Metal in future
            //spirv_cross::CompilerGLSL glsl_compiler(handle.SPIRV);
            //std::string glsl_code = glsl_compiler.compile();
            //Log<Severity::Info>("GLSL Code:\n{0}", glsl_code);			
            // 
            //spirv_cross::CompilerHLSL hlsl_compiler(handle.SPIRV);
            //spirv_cross::CompilerHLSL::Options hlsl_options;
            //hlsl_options.shader_model = 50;
            //hlsl_compiler.set_hlsl_options(hlsl_options);
            //std::string hlsl_code = hlsl_compiler.compile();
            //Log<Severity::Info>("HLSL Code:\n{0}", hlsl_code);
            // 
            //spirv_cross::CompilerMSL msl_compiler(handle.SPIRV);
            //std::string msl_code = msl_compiler.compile();
            //Log<Severity::Info>("MSL Code:\n{0}", msl_code);
        }

        result.ClearRepeatedMembers();
        return result;
    }

} // namespace Surge