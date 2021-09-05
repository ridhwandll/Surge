// Copyright (c) - SurgeTechnologies - All rights reserved
#include "ShaderReflector.hpp"
#include <SPIRV-Cross/spirv_glsl.hpp>

namespace Surge
{
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
            res.Name = resource.name;
            result.PushResource(res);
        }

        // Fetch all the Uniform/Constannt buffers
        for (const spirv_cross::Resource& resource : resources.uniform_buffers)
        {
            ShaderBuffer buffer;
            const spirv_cross::SPIRType& bufferType = compiler.get_type(resource.base_type_id);

            buffer.Size = static_cast<Uint>(compiler.get_declared_struct_size(bufferType));
            buffer.Binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
            buffer.BufferName = resource.name;

            for (Uint i = 0; i < bufferType.member_types.size(); i++)
            {
                ShaderBufferMember bufferMember;
                bufferMember.Name = buffer.BufferName + '.' + compiler.get_member_name(bufferType.self, i);
                bufferMember.MemoryOffset = compiler.type_struct_member_offset(bufferType, i); // In bytes
                buffer.Members.emplace_back(bufferMember);

                Log("Name: {0} | Memory Offset: {1} | Binding: {2}", bufferMember.Name, bufferMember.MemoryOffset, buffer.Binding);
            }

    #ifdef SURGE_DEBUG
            result.ValidateBuffer(buffer);
    #endif
            result.PushBuffer(buffer);
        }

        return result;
    }
}
