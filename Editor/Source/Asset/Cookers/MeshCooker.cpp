// Copyright (c) - SurgeTechnologies - All rights reserved
#include "MeshCooker.hpp"
#include "Surge/Core/Core.hpp"
#include "Surge/Utility/Filesystem.hpp"
#include "Surge/Graphics/Renderer/Renderer.hpp"
#include "Surge/Asset/AssetManager.hpp"
#include "Surge/Asset/Serializer/Mesh/MeshBinaryFormat.hpp"

#include <cgltf.h>
#include <meshoptimizer.h>
#include <glm/gtc/type_ptr.hpp>

namespace Surge
{
    static cgltf_data* ParseGLTF(const String& filepath)
    {
        cgltf_options options = {};

        options.file.read = [](const cgltf_memory_options*, const cgltf_file_options*, const char* path, cgltf_size* size, void** data) -> cgltf_result {
            Vector<Byte> buffer;
            if(!Filesystem::ReadBinaryFile(path, buffer))
                return cgltf_result_file_not_found;

            *size = buffer.size();
            *data = malloc(*size);
            if(!*data)
                return cgltf_result_out_of_memory;

            memcpy(*data, buffer.data(), *size);
            return cgltf_result_success;
            };

        options.file.release = [](const cgltf_memory_options*, const cgltf_file_options*, void* data, cgltf_size) { free(data); };

        cgltf_data* data = nullptr;
        if(cgltf_parse_file(&options, filepath.c_str(), &data) != cgltf_result_success)
        {
            Log<Severity::Error>("[MeshSerializer] Failed to parse glTF: {}", filepath);
            return nullptr;
        }
        if(cgltf_load_buffers(&options, data, filepath.c_str()) != cgltf_result_success)
        {
            Log<Severity::Error>("[MeshSerializer] Failed to load buffers: {}", filepath);
            cgltf_free(data);
            return nullptr;
        }
        return data;
    }

    static MeshSpecification ExtractGeometry(cgltf_data* data, MeshSpecification& outSpec)
    {
        Vector<Uint> meshSubmeshStart(data->meshes_count);
        Uint totalSubmeshes = 0;
        for(size_t i = 0; i < data->meshes_count; i++)
        {
            meshSubmeshStart[i] = totalSubmeshes;
            totalSubmeshes += (Uint)data->meshes[i].primitives_count;
        }
        outSpec.Submeshes.reserve(totalSubmeshes);

        Uint vertexCount = 0;
        Uint indexCount = 0;

        for(size_t m = 0; m < data->meshes_count; m++)
        {
            cgltf_mesh& mesh = data->meshes[m];
            for(size_t p = 0; p < mesh.primitives_count; p++)
            {
                cgltf_primitive& prim = mesh.primitives[p];
                SG_ASSERT(prim.type == cgltf_primitive_type_triangles, "Only triangle primitives supported");
                SG_ASSERT(prim.indices, "Mesh primitive requires indices");

                cgltf_accessor* posAccessor = nullptr;
                cgltf_accessor* normalAccessor = nullptr;
                cgltf_accessor* tangentAccessor = nullptr;
                cgltf_accessor* texcoordAccessor = nullptr;
                [[maybe_unused]] cgltf_accessor* colorAccessor = nullptr;

                for(size_t a = 0; a < prim.attributes_count; a++)
                {
                    cgltf_attribute& attr = prim.attributes[a];
                    switch(attr.type)
                    {
                        case cgltf_attribute_type_position: posAccessor = attr.data; break;
                        case cgltf_attribute_type_normal: normalAccessor = attr.data; break;
                        case cgltf_attribute_type_tangent: tangentAccessor = attr.data; break;
                        case cgltf_attribute_type_color: colorAccessor = attr.data; break;
                        case cgltf_attribute_type_texcoord:
                            if(attr.index == 0) texcoordAccessor = attr.data;
                            break;
                        default:
                            break;
                    }
                }

                SG_ASSERT(posAccessor, "Mesh primitive requires positions");
                SG_ASSERT(normalAccessor, "Mesh primitive requires normals");

                outSpec.Submeshes.emplace_back();
                Submesh& submesh = outSpec.Submeshes.back();
                submesh.BaseVertex = vertexCount;
                submesh.BaseIndex = indexCount;
                submesh.VertexCount = (Uint)posAccessor->count;
                submesh.IndexCount = (Uint)prim.indices->count;
                submesh.MaterialIndex = prim.material ? (Uint)(prim.material - data->materials) : 0;
                submesh.MeshName = mesh.name ? mesh.name : "Unnamed";
                submesh.BoundingBox.Reset();

                SG_ASSERT(submesh.IndexCount % 3 == 0, "Index count must be a multiple of 3");

                for(size_t i = 0; i < posAccessor->count; i++)
                {
                    outSpec.Vertices.emplace_back();
                    Vertex& v = outSpec.Vertices.back();

                    cgltf_accessor_read_float(posAccessor, i, &v.Position.x, 3);
                    cgltf_accessor_read_float(normalAccessor, i, &v.Normal.x, 3);

                    if(tangentAccessor)
                    {
                        float t[4];
                        cgltf_accessor_read_float(tangentAccessor, i, t, 4);
                        v.Tangent = { t[0], t[1], t[2] };
                        v.Bitangent = glm::cross(v.Normal, v.Tangent) * t[3];
                    }

                    if(texcoordAccessor)
                        cgltf_accessor_read_float(texcoordAccessor, i, &v.TexCoord.x, 2);

                    // Vertex Color, once used it for AO
                    //if(colorAccessor)
                    //{
                    //    float col[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
                    //    cgltf_accessor_read_float(colorAccessor, i, col, (colorAccessor->type == cgltf_type_vec3) ? 3 : 4);
                    //    v.Color = glm::packUnorm4x8(glm::vec4(col[0], col[1], col[2], col[3]));
                    //}
                    //else
                    //{
                    //    // Default 1.0 AO / White tint
                    //    v.Color = 0xFFFFFFFF;
                    //}

                    submesh.BoundingBox.Min = glm::min(v.Position, submesh.BoundingBox.Min);
                    submesh.BoundingBox.Max = glm::max(v.Position, submesh.BoundingBox.Max);
                }

                for(size_t i = 0; i < prim.indices->count; i += 3)
                {
                    outSpec.Indices.push_back({
                        (Uint)cgltf_accessor_read_index(prim.indices, i),
                        (Uint)cgltf_accessor_read_index(prim.indices, i + 1),
                        (Uint)cgltf_accessor_read_index(prim.indices, i + 2) });
                }

                vertexCount += submesh.VertexCount;
                indexCount += submesh.IndexCount;
            }
        }

        for(size_t n = 0; n < data->nodes_count; n++)
        {
            cgltf_node& node = data->nodes[n];
            if(!node.mesh) continue;

            float worldMat[16];
            cgltf_node_transform_world(&node, worldMat);
            glm::mat4 worldTransform = glm::make_mat4(worldMat);

            float localMat[16];
            cgltf_node_transform_local(&node, localMat);
            glm::mat4 localTransform = glm::make_mat4(localMat);

            size_t meshIdx = node.mesh - data->meshes;
            Uint startSubmesh = meshSubmeshStart[meshIdx];

            for(size_t p = 0; p < node.mesh->primitives_count; p++)
            {
                Submesh& submesh = outSpec.Submeshes[startSubmesh + p];
                submesh.NodeName = node.name ? node.name : "Unnamed";
                submesh.Transform = worldTransform;
                submesh.LocalTransform = localTransform;
            }
        }

        return outSpec;
    }

    static const char* GetImageExtension(const Byte* bytes, size_t size)
    {
        if(size >= 8 && memcmp(bytes, "\x89PNG", 4) == 0)
            return ".png";
        if(size >= 3 && bytes[0] == 0xFF && bytes[1] == 0xD8 && bytes[2] == 0xFF)
            return ".jpg";
        return nullptr;
    }

    // Leaves an identical file untouched: its mod time is part of the texture's AssetStamp, rewriting it would re-cook the texture
    static bool WriteFileIfChanged(const Path& path, const Byte* bytes, size_t size)
    {
        Vector<Byte> existing;
        if(Filesystem::Exists(path) && Filesystem::ReadBinaryFile(path, existing) && existing.size() == size && memcmp(existing.data(), bytes, size) == 0)
            return true;

        return Filesystem::WriteBinaryFile(path, bytes, size);
    }

    // Embedded images (.glb buffer views, base64 data URIs) have no source file the texture pipeline could cook, so they are written next to
    // the mesh as "<Mesh>_Textures/Image<N>_<Usage>.png|jpg" and imported like any other texture. The usage suffix picks the color space
    // (Texture2DCooker::IsLinearColorSpace reads the file name). Returns the absolute path, empty on failure
    static Path ExtractEmbeddedImage(const Path& meshPath, cgltf_data* data, cgltf_image* image, const char* usage)
    {
        const Uint imageIndex = static_cast<Uint>(cgltf_image_index(data, image));
        const Byte* bytes = nullptr;
        size_t size = 0;
        void* decoded = nullptr;

        if(image->buffer_view)
        {
            bytes = cgltf_buffer_view_data(image->buffer_view);
            size = image->buffer_view->size;
        }
        else // data:image/png;base64,...
        {
            const char* comma = strchr(image->uri, ',');
            if(!comma || !strstr(image->uri, ";base64"))
            {
                Log<Severity::Warn>("[MeshCooker] Image {} of '{}' has an unsupported data URI, skipped", imageIndex, meshPath.string());
                return {};
            }

            const char* base64 = comma + 1;
            const size_t length = strlen(base64);
            size_t padding = 0;
            while(padding < 2 && padding < length && base64[length - 1 - padding] == '=')
                padding++;

            size = length / 4 * 3;
            size = size > padding ? size - padding : 0;
            cgltf_options options = {};
            if(size == 0 || cgltf_load_buffer_base64(&options, size, base64, &decoded) != cgltf_result_success)
            {
                Log<Severity::Warn>("[MeshCooker] Image {} of '{}' has a malformed data URI, skipped", imageIndex, meshPath.string());
                return {};
            }
            bytes = static_cast<const Byte*>(decoded);
        }

        Path imagePath;
        const char* extension = bytes ? GetImageExtension(bytes, size) : nullptr;
        if(extension)
        {
            const Path directory = Filesystem::GetParentPath(meshPath) / (Filesystem::GetFilenameWithoutExt(meshPath) + "_Textures");
            imagePath = directory / std::format("Image{}_{}{}", imageIndex, usage, extension);

            Filesystem::CreateOrEnsureDirectories(directory);
            if(!Filesystem::Exists(directory) || !WriteFileIfChanged(imagePath, bytes, size))
            {
                Log<Severity::Error>("[MeshCooker] Failed to write the embedded image '{}'", imagePath.string());
                imagePath.clear();
            }
        }
        else
            Log<Severity::Warn>("[MeshCooker] Image {} of '{}' is not a PNG or JPEG ({}), skipped", imageIndex, meshPath.string(), image->mime_type ? image->mime_type : "unknown type");

        free(decoded); // (Rid IMPORTANT) cgltf's default allocator is malloc Look here if someone ever implements a custom allocator that doesn't use malloc/free
        return imagePath;
    }

    static void ExtractMaterials(const String& absPath, cgltf_data* data, MeshSpecification& spec)
    {
        AssetManager* assetManager = Core::GetAssetManager();
        bool importedTextures = false;

        auto SetMaterialTexture = [&](const Path& meshPath, cgltf_texture_view& texView, Ref<Material>& material, const String& texName, const char* usage) -> bool {
            if(!texView.texture || !texView.texture->image)
                return false;

            cgltf_image* image = texView.texture->image;

            Path absTexturePath;
            if(image->buffer_view || (image->uri && strncmp(image->uri, "data:", 5) == 0))
                absTexturePath = ExtractEmbeddedImage(meshPath, data, image, usage);
            else if(image->uri)
            {
                String uri = image->uri;
                uri.resize(cgltf_decode_uri(uri.data())); // "My%20Texture.png" -> "My Texture.png"
                absTexturePath = (Filesystem::GetParentPath(meshPath) / uri).lexically_normal();
            }

            if(absTexturePath.empty())
                return false;

            const Path relTexturePath = absTexturePath.lexically_relative(Path(assetManager->GetAssetsDirectory()));
            if(relTexturePath.empty() || *relTexturePath.begin() == "..")
            {
                Log<Severity::Warn>("[Mesh] Texture '{}' is outside the assets directory, skipping.", absTexturePath.string());
                return false;
            }

            const String relativePath = relTexturePath.generic_string();
            if(!assetManager->GetIDFromPath(relativePath).IsValid())
                importedTextures = true;
            const AssetID texID = assetManager->Import(relativePath, AssetType::TEXTURE2D);

            if(!texID.IsValid())
                return false;

            material->SetTexture(texName, assetManager->Load<Texture2D>(texID));
            return true;
            };

        if(data->materials_count > 0)
        {
            spec.Materials.resize(data->materials_count);

            for(size_t i = 0; i < data->materials_count; i++)
            {
                cgltf_material& mat = data->materials[i];
                const String materialName = mat.name ? mat.name : "Unnamed";

                Ref<Material> material = Material::Create(materialName);
                spec.Materials[i] = material;

                ImageHandle whiteTexture = Core::GetRenderer()->GetWhiteTexture();
                material->SetTexture("AlbedoMap", whiteTexture);
                material->SetTexture("NormalMap", whiteTexture);
                material->SetTexture("RoughnessMetallicMap", whiteTexture);
                material->SetTexture("EmissionMap", whiteTexture);

                if(mat.has_pbr_metallic_roughness)
                {
                    auto& pbr = mat.pbr_metallic_roughness;
                    material->Set<glm::vec3>("Albedo", glm::vec3(pbr.base_color_factor[0], pbr.base_color_factor[1], pbr.base_color_factor[2]));
                    material->Set<float>("Roughness", pbr.roughness_factor);
                    material->Set<float>("Metallic", pbr.metallic_factor);
                    material->Set<float>("Reflectance", 0.5f);

                    if(SetMaterialTexture(absPath, pbr.base_color_texture, material, "AlbedoMap", "BaseColor"))
                        material->Set<int>("UseAlbedoMap", 1);
                    if(SetMaterialTexture(absPath, pbr.metallic_roughness_texture, material, "RoughnessMetallicMap", "MetallicRoughness"))
                    {
                        material->Set<int>("UseRoughnessMap", 1);
                        material->Set<int>("UseMetallicMap", 1);
                    }
                }
                if(SetMaterialTexture(absPath, mat.normal_texture, material, "NormalMap", "Normal"))
                    material->Set<int>("UseNormalMap", 1);
                else
                    material->Set<int>("UseNormalMap", 0);

                material->Set<glm::vec3>("Emission", glm::vec3(mat.emissive_factor[0], mat.emissive_factor[1], mat.emissive_factor[2]));
                float emissiveStrength = mat.has_emissive_strength ? mat.emissive_strength.emissive_strength : 1.0f;
                material->Set<float>("EmissionIntensity", emissiveStrength);
                if(SetMaterialTexture(absPath, mat.emissive_texture, material, "EmissionMap", "Emission"))
                    material->Set<int>("UseEmissionMap", 1);
            }
        }

        // The mesh binary references the textures by ID, which must survive an Editor restart
        if(importedTextures)
            assetManager->SerializeRegistry();
    }

    static void FreeGLTF(cgltf_data* data)
    {
        cgltf_free(data);
    }

    static void OptimizeMesh(MeshSpecification& spec)
    {
        for(Submesh& sm : spec.Submeshes)
        {
            if(sm.IndexCount == 0 || sm.VertexCount == 0)
                continue;

            Uint* indices = reinterpret_cast<Uint*>(spec.Indices.data()) + sm.BaseIndex;
            Vertex* vertices = spec.Vertices.data() + sm.BaseVertex;

            Vector<Uint> remap(sm.VertexCount);
            size_t uniqueVerts = meshopt_generateVertexRemap(remap.data(), indices, sm.IndexCount, vertices, sm.VertexCount, sizeof(Vertex));

            if (sm.VertexCount != uniqueVerts)
                Log<Severity::Warn>("[MeshCooker] Old Vertex Count: {} | New Vertex Count: {}", sm.VertexCount, uniqueVerts);

            meshopt_remapIndexBuffer(indices, indices, sm.IndexCount, remap.data());
            meshopt_remapVertexBuffer(vertices, vertices, sm.VertexCount, sizeof(Vertex), remap.data());
            meshopt_optimizeVertexCache(indices, indices, sm.IndexCount, uniqueVerts);
            meshopt_optimizeOverdraw(indices, indices, sm.IndexCount, &vertices[0].Position.x, uniqueVerts, sizeof(Vertex), 1.05f);
            meshopt_optimizeVertexFetch(vertices, indices, sm.IndexCount, vertices, uniqueVerts, sizeof(Vertex));

            sm.VertexCount = uniqueVerts;
        }
    }

    CookResult MeshCooker::Cook(const String & sourceAbsPath, AssetID id) const
    {
        cgltf_data* data = ParseGLTF(sourceAbsPath);
        if(!data)
            return {};

        AssetManager* am = Core::GetAssetManager();

        MeshSpecification spec;
        ExtractGeometry(data, spec);
        ExtractMaterials(sourceAbsPath, data, spec);
        FreeGLTF(data);
        OptimizeMesh(spec);

        const AssetStamp stamp = AssetStampWriter::Build(sourceAbsPath, GetCookerVersion());

        const String sidecarPath = am->GetSidecarPath(id);
        bool res = MeshBinary::Write(sidecarPath, stamp, spec, {});

        CookResult result;
        result.Success = res;
        result.OutputPath = sidecarPath;
        result.InputMegaBytes = Filesystem::FileSizeInMB(sourceAbsPath);
        result.OutputMegaBytes = Filesystem::FileSizeInMB(sidecarPath);
        return result;
    }
}
