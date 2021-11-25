// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "SurgeMath/AABB.hpp"
#include "Surge/Graphics/Interface/GraphicsPipeline.hpp"
#include "Surge/Graphics/Interface/IndexBuffer.hpp"
#include "Surge/Graphics/Interface/VertexBuffer.hpp"
#include <glm/glm.hpp>

struct aiMesh;
struct aiNode;

namespace Surge
{
    struct Vertex
    {
        glm::vec3 Position;
        glm::vec3 Normal;
        glm::vec3 Tangent;
        glm::vec3 Bitangent;
        glm::vec2 TexCoord;
    };

    struct Submesh
    {
        Uint BaseVertex;
        Uint BaseIndex;
        Uint MaterialIndex;

        Uint IndexCount;
        Uint VertexCount;
        AABB BoundingBox;

        glm::mat4 Transform;
        glm::mat4 LocalTransform;
        String NodeName, MeshName;
    };

    struct Index
    {
        Uint V1, V2, V3;
    };

    class Mesh : public RefCounted
    {
    public:
        Mesh(const Path& filepath);

        // Returns the path from which the Mesh was loaded
        FORCEINLINE const Path& GetPath() const { return mPath; }

        // Returns the vertex buffer of the mesh
        FORCEINLINE const Ref<VertexBuffer>& GetVertexBuffer() const { return mVertexBuffer; }

        // Returns the index buffer of the mesh
        FORCEINLINE const Ref<IndexBuffer>& GetIndexBuffer() const { return mIndexBuffer; }

        // Returns the submeshes of the mesh/model
        FORCEINLINE const Vector<Submesh>& GetSubmeshes() const { return mSubmeshes; }

    private:
        void GetVertexData(const aiMesh* mesh, AABB& outAABB);
        void GetIndexData(const aiMesh* mesh);
        void TraverseNodes(aiNode* node, const glm::mat4& parentTransform = glm::mat4(1.0f), Uint level = 0);

    private:
        Path mPath;
        Vector<Submesh> mSubmeshes;

        Ref<VertexBuffer> mVertexBuffer;
        Ref<IndexBuffer> mIndexBuffer;

        Vector<Vertex> mVertices;
        Vector<Index> mIndices;
    };

} // namespace Surge