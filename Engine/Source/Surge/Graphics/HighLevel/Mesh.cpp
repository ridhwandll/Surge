// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Mesh.hpp"
#include "Surge/Core/Core.hpp"
#include "Surge/Graphics/Renderer/Renderer.hpp"
#include "Surge/Asset/AssetManager.hpp"
#include "Material.hpp"

namespace Surge
{
    Mesh::Mesh(MeshSpecification&& spec)
    {
        // Culling relies on per-submesh bounds, rebuild them from the vertices (meshes created in code never fill them)
        for(Submesh& submesh : spec.Submeshes)
        {
            submesh.BoundingBox.Reset();
            const Uint end = std::min(submesh.BaseVertex + submesh.VertexCount, static_cast<Uint>(spec.Vertices.size()));
            for(Uint v = submesh.BaseVertex; v < end; v++)
            {
                submesh.BoundingBox.Min = glm::min(submesh.BoundingBox.Min, spec.Vertices[v].Position);
                submesh.BoundingBox.Max = glm::max(submesh.BoundingBox.Max, spec.Vertices[v].Position);
            }

            if(submesh.BoundingBox.Min.x > submesh.BoundingBox.Max.x) // No vertices, never culled
                submesh.BoundingBox = AABB(glm::vec3(-FLT_MAX * 0.5f), glm::vec3(FLT_MAX * 0.5f));
        }

        Scope<GraphicsRHI>& rhi = Core::GetRenderer()->GetRHI();
        BufferDesc vbDesc = {};
        vbDesc.Size = static_cast<Uint>(spec.Vertices.size()) * sizeof(Vertex);
        vbDesc.Usage = BufferUsage::VERTEX;
        vbDesc.HostVisible = false;
        vbDesc.InitialData = spec.Vertices.data();
        vbDesc.DebugName = "MeshVB";
        mVertexBuffer = rhi->CreateBuffer(vbDesc);
        BufferDesc ibDesc = {};
        ibDesc.Size = static_cast<Uint>(spec.Indices.size()) * sizeof(Index);
        ibDesc.Usage = BufferUsage::INDEX;
        ibDesc.HostVisible = false;
        ibDesc.InitialData = spec.Indices.data();
        ibDesc.DebugName = "MeshIB";
        mIndexBuffer = rhi->CreateBuffer(ibDesc);

        mSubmeshes = std::move(spec.Submeshes);
        mGLTFMaterials = std::move(spec.Materials);

        mMaterialOverrides.resize(mGLTFMaterials.size(), nullptr);

        // Convert stored AssetIDs to live Refs
        AssetManager* am = Core::GetAssetManager();
        for(Uint i = 0; i < (Uint)spec.MaterialOverrides.size() && i < (Uint)mGLTFMaterials.size(); i++)
        {
            const AssetID& id = spec.MaterialOverrides[i];
            if(id.IsValid())
            {
                Ref<Material> material = am->Load<Material>(id);
                if (material)
                {
                    mMaterialOverrides[i] = material;
                    mGLTFMaterials[i].Reset(); // (Rid)Release the transient material loaded from glTF, since we have a user override for this slot
                }
                else
                    Log<Severity::Warn>("[Mesh] Failed to load material override for mesh at index {}! The material is missing probably, falling back to transient material!", i);
            }
        }

        //TODO: Remove
        mVertices = std::move(spec.Vertices);
        mIndices = std::move(spec.Indices);
    }

    Mesh::~Mesh()
    {
        // Destroy* is deferred until the GPU has retired every frame that may still use the buffers, no device drain needed
        Scope<GraphicsRHI>& rhi = Core::GetRenderer()->GetRHI();
        rhi->DestroyBuffer(mVertexBuffer);
        rhi->DestroyBuffer(mIndexBuffer);
    }

    Ref<Mesh> Mesh::Create(MeshSpecification&& spec)
    {
        return Ref<Mesh>::Create(std::move(spec));
    }

    const Material* Mesh::GetMaterialPtrAtIndex(Uint index) const
    {
        SG_ASSERT(index < mGLTFMaterials.size(), "Material index out of range!");

        if(index < mMaterialOverrides.size() && mMaterialOverrides[index])
            return mMaterialOverrides[index].Raw();

        return mGLTFMaterials[index].Raw();
    }

    Ref<Material> Mesh::GetMaterialAtIndex(Uint index) const
    {
        SG_ASSERT(index < mGLTFMaterials.size(), "Material index out of range!");

        if(index < mMaterialOverrides.size() && mMaterialOverrides[index])
            return mMaterialOverrides[index]; // Actual user override, owned by user, created from editor or runtime

        return mGLTFMaterials[index]; // Transient material from mesh, not owned by anyone, created from glTF
    }

} // namespace Surge