// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Mesh.hpp"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace Surge
{
    glm::mat4 AssimpMat4ToGlmMat4(const aiMatrix4x4& matrix)
    {
        glm::mat4 result;
        result[0][0] = matrix.a1; result[1][0] = matrix.a2; result[2][0] = matrix.a3; result[3][0] = matrix.a4;
        result[0][1] = matrix.b1; result[1][1] = matrix.b2; result[2][1] = matrix.b3; result[3][1] = matrix.b4;
        result[0][2] = matrix.c1; result[1][2] = matrix.c2; result[2][2] = matrix.c3; result[3][2] = matrix.c4;
        result[0][3] = matrix.d1; result[1][3] = matrix.d2; result[2][3] = matrix.d3; result[3][3] = matrix.d4;
        return result;
    }

    static const Uint sMeshImportFlags =
        aiProcess_Triangulate |
        aiProcess_GenNormals |
        aiProcess_GenUVCoords |
        aiProcess_OptimizeMeshes |
        aiProcess_ValidateDataStructure |
        aiProcess_JoinIdenticalVertices |
        aiProcess_CalcTangentSpace;

    Mesh::Mesh(const String& filepath)
        : mPath(filepath)
    {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(filepath, sMeshImportFlags);

        if (!scene || !scene->HasMeshes())
            Log<Severity::Error>("Failed to load mesh file: {0}", filepath);

        Uint vertexCount = 0;
        Uint indexCount = 0;

        mSubmeshes.reserve(scene->mNumMeshes);
        for (size_t m = 0; m < scene->mNumMeshes; m++)
        {
            aiMesh* mesh = scene->mMeshes[m];
            SG_ASSERT(mesh->HasPositions(), "Meshes require positions.");
            SG_ASSERT(mesh->HasNormals(), "Meshes require normals.");

            Submesh& submesh = mSubmeshes.emplace_back();
            submesh.BaseVertex = vertexCount;
            submesh.BaseIndex = indexCount;
            submesh.MaterialIndex = mesh->mMaterialIndex;
            submesh.IndexCount = mesh->mNumFaces * 3;
            submesh.VertexCount = mesh->mNumVertices;
            submesh.MeshName = mesh->mName.C_Str();
            vertexCount += submesh.VertexCount;
            indexCount += submesh.IndexCount;

            GetVertexData(mesh, submesh.BoundingBox);
            GetIndexData(mesh);
        }

        TraverseNodes(scene->mRootNode);

        //TODO: Materials here

        mVertexBuffer = VertexBuffer::Create(mVertices.data(), static_cast<Uint>(mVertices.size()) * sizeof(Vertex));
        mIndexBuffer = IndexBuffer::Create(mIndices.data(), static_cast<Uint>(mIndices.size() * sizeof(Index)));

        GraphicsPipelineSpecification pipelineSpec{};
        pipelineSpec.Shader = CoreGetRenderer()->GetShader("Simple"); //TODO: Should be handled by material
        pipelineSpec.Topology = PrimitiveTopology::TriangleStrip;
        pipelineSpec.UseDepth = true;
        pipelineSpec.UseStencil = false;
        pipelineSpec.DebugName = "MeshPipeline";
        pipelineSpec.LineWidth = 1.0f;
        mPipeline = GraphicsPipeline::Create(pipelineSpec);
    }

    void Mesh::GetVertexData(const aiMesh* mesh, AABB& outAABB)
    {
        outAABB.Reset();
        for (size_t i = 0; i < mesh->mNumVertices; i++)
        {
            Vertex vertex;
            vertex.Position = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };
            vertex.Normal = { mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };

            outAABB.Min.x = glm::min(vertex.Position.x, outAABB.Min.x);
            outAABB.Min.y = glm::min(vertex.Position.y, outAABB.Min.y);
            outAABB.Min.z = glm::min(vertex.Position.z, outAABB.Min.z);
            outAABB.Max.x = glm::max(vertex.Position.x, outAABB.Max.x);
            outAABB.Max.y = glm::max(vertex.Position.y, outAABB.Max.y);
            outAABB.Max.z = glm::max(vertex.Position.z, outAABB.Max.z);

            if (mesh->HasTangentsAndBitangents())
            {
                vertex.Tangent = { mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z };
                vertex.Bitangent = { mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z };
            }

            if (mesh->HasTextureCoords(0))
                vertex.TexCoord = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
            else
                vertex.TexCoord = { 0.0f, 0.0f };

            mVertices.push_back(vertex);
        }

    }

    void Mesh::GetIndexData(const aiMesh* mesh)
    {
        for (size_t i = 0; i < mesh->mNumFaces; i++)
        {
            SG_ASSERT(mesh->mFaces[i].mNumIndices == 3, "Mesh Must have 3 indices!");
            Index index = { mesh->mFaces[i].mIndices[0], mesh->mFaces[i].mIndices[1], mesh->mFaces[i].mIndices[2] };
            mIndices.push_back(index);
        }
    }

    void Mesh::TraverseNodes(aiNode* node, const glm::mat4& parentTransform, Uint level)
    {
        const glm::mat4 localTransform = AssimpMat4ToGlmMat4(node->mTransformation);
        const glm::mat4 transform = parentTransform * localTransform;

        for (Uint i = 0; i < node->mNumMeshes; i++)
        {
            const Uint mesh = node->mMeshes[i];
            auto& submesh = mSubmeshes[mesh];
            submesh.NodeName = node->mName.C_Str();
            submesh.Transform = transform;
            submesh.LocalTransform = localTransform;
        }

        for (Uint i = 0; i < node->mNumChildren; i++)
            TraverseNodes(node->mChildren[i], transform, level + 1);
    }
}
