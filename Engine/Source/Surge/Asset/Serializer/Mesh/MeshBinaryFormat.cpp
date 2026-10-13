// Copyright (c) - SurgeTechnologies - All rights reserved
#include "MeshBinaryFormat.hpp"
#include "Surge/Utility/Filesystem.hpp"
#include "Surge/Asset/Serializer/BinaryHelpers.hpp"
#include "Surge/Asset/Serializer/Material/MaterialBinaryFormat.hpp"

namespace Surge::MeshBinary
{
    static_assert(std::is_trivially_copyable_v<Vertex>, "Vertex must be trivially copyable for binary sidecar serialization");
    static_assert(std::is_trivially_copyable_v<Index>, "Index must be trivially copyable for binary sidecar serialization");
    static constexpr Uint kSidecarMagic = 0x4D444952; // RIDM
    static constexpr Uint kSidecarVersion = 2;
    struct Header
    {
        Uint Magic;
        Uint Version;
        Uint VertexCount;
        Uint IndexCount;
        Uint SubmeshCount;
        Uint TransientMaterialCount;
        Uint ValidOverrideCount;
        Uint GeomSectionSize;
    };
    static_assert(sizeof(Header) == 32);

    bool Write(const String& path, const AssetStamp& stamp, const MeshSpecification& spec, const Vector<Ref<Material>>& overrides)
    {
        Uint validOverrideCount = 0;
        for(const Ref<Material>& ref : overrides)
        {
            if(ref)
                validOverrideCount++;
        }

        static constexpr Uint sSubmeshPODSize =
            5 * sizeof(Uint) +       // BaseVertex/BaseIndex/MaterialIndex/IndexCount/VertexCount
            2 * sizeof(glm::vec3) +  // BoundsMin + BoundsMax
            2 * sizeof(glm::mat4);   // Transform + LocalTransform

        Uint geomSize = 0;
        geomSize += static_cast<Uint>(spec.Vertices.size()) * sizeof(Vertex);
        geomSize += static_cast<Uint>(spec.Indices.size()) * sizeof(Index);
        for(const Submesh& sm : spec.Submeshes)
        {
            geomSize += sSubmeshPODSize;
            geomSize += sizeof(Uint) + static_cast<Uint>(sm.NodeName.size());
            geomSize += sizeof(Uint) + static_cast<Uint>(sm.MeshName.size());
        }
        for(const Ref<Material>& mat : spec.Materials)
            geomSize += MaterialBinary::CalculateSize(mat);

        Vector<Byte> buffer;
        buffer.reserve(sizeof(AssetStamp) + sizeof(Header) + geomSize + validOverrideCount * (sizeof(Uint) + sizeof(uint64_t)));

        Header header = {};
        header.Magic = kSidecarMagic;
        header.Version = kSidecarVersion;
        header.VertexCount = static_cast<Uint>(spec.Vertices.size());
        header.IndexCount = static_cast<Uint>(spec.Indices.size());
        header.SubmeshCount = static_cast<Uint>(spec.Submeshes.size());
        header.TransientMaterialCount = static_cast<Uint>(spec.Materials.size());
        header.ValidOverrideCount = validOverrideCount;
        header.GeomSectionSize = geomSize;

        WriteData(buffer, stamp);
        WriteData(buffer, header);
        WriteDataArray(buffer, spec.Vertices.data(), spec.Vertices.size());
        WriteDataArray(buffer, spec.Indices.data(), spec.Indices.size());

        for(const Submesh& sm : spec.Submeshes)
        {
            WriteData(buffer, sm.BaseVertex);
            WriteData(buffer, sm.BaseIndex);
            WriteData(buffer, sm.MaterialIndex);
            WriteData(buffer, sm.IndexCount);
            WriteData(buffer, sm.VertexCount);
            WriteData(buffer, sm.BoundingBox.Min);
            WriteData(buffer, sm.BoundingBox.Max);
            WriteData(buffer, sm.Transform);
            WriteData(buffer, sm.LocalTransform);
            WriteStr(buffer, sm.NodeName);
            WriteStr(buffer, sm.MeshName);
        }

        for(const Ref<Material>& mat : spec.Materials)
            MaterialBinary::WriteBuffer(buffer, mat);

        for(Uint i = 0; i < static_cast<Uint>(overrides.size()); i++)
        {
            if(!overrides[i]) continue;
            Uint slotIndex = i;
            uint64_t rawID = overrides[i]->GetID().Get();
            WriteData(buffer, slotIndex);
            WriteData(buffer, rawID);
        }

        if(!Filesystem::WriteBinaryFile(path, buffer.data(), buffer.size()))
        {
            Log<Severity::Error>("[MeshBinary] Failed to write sidecar: {}", path);
            return false;
        }
        Log<Severity::Trace>("[MeshBinary] Cooked sidecar: {}", path);
        return true;
    }

    bool Read(const String& path, AssetStamp& outstamp, MeshSpecification& outSpec)
    {
        Vector<Byte> fileData;
        if(!Filesystem::ReadBinaryFile(path, fileData))
            return false;

        if(fileData.size() < sizeof(AssetStamp) + sizeof(Header))
            return false;

        BinaryReader reader(fileData.data(), fileData.size());
        reader.Read(outstamp);

        Header header = {};
        reader.Read(header);

        if(header.Magic != kSidecarMagic)
        {
            Log<Severity::Warn>("[MeshSerializer] {} has bad magic, ignoring sidecar.", path);
            return false;
        }
        if(header.Version != kSidecarVersion)
        {
            Log<Severity::Warn>("[MeshSerializer] {} version mismatch (got {}, want {}), re-cooking!", path, header.Version, kSidecarVersion);
            return false;
        }

        // Every count comes from the file: check it against the bytes left before allocating from it (each entry is at least one byte)
        if(!reader.Fits(header.VertexCount, sizeof(Vertex)) || !reader.Fits(header.IndexCount, sizeof(Index)) ||
           !reader.Fits(header.SubmeshCount, 1) || !reader.Fits(header.TransientMaterialCount, 1))
        {
            Log<Severity::Error>("[MeshSerializer] {} is truncated or corrupted!", path);
            return false;
        }

        outSpec.Vertices.resize(header.VertexCount);
        reader.ReadArray(outSpec.Vertices.data(), header.VertexCount);

        outSpec.Indices.resize(header.IndexCount);
        reader.ReadArray(outSpec.Indices.data(), header.IndexCount);

        outSpec.Submeshes.reserve(header.SubmeshCount);
        for(Uint i = 0; i < header.SubmeshCount && !reader.Failed; i++)
        {
            Submesh& sm = outSpec.Submeshes.emplace_back();
            reader.Read(sm.BaseVertex);
            reader.Read(sm.BaseIndex);
            reader.Read(sm.MaterialIndex);
            reader.Read(sm.IndexCount);
            reader.Read(sm.VertexCount);
            reader.Read(sm.BoundingBox.Min);
            reader.Read(sm.BoundingBox.Max);
            reader.Read(sm.Transform);
            reader.Read(sm.LocalTransform);
            sm.NodeName = reader.ReadStr();
            sm.MeshName = reader.ReadStr();
        }

        outSpec.Materials.resize(header.TransientMaterialCount);
        for(Uint i = 0; i < header.TransientMaterialCount && !reader.Failed; i++)
            outSpec.Materials[i] = MaterialBinary::ReadBuffer(reader);

        outSpec.MaterialOverrides.assign(outSpec.Submeshes.size(), AssetID::INVALID);
        for(Uint i = 0; i < header.ValidOverrideCount && !reader.Failed; i++)
        {
            Uint slotIndex = 0;
            uint64_t rawID = 0;
            reader.Read(slotIndex);
            reader.Read(rawID);
            if(!reader.Failed && slotIndex < static_cast<Uint>(outSpec.Submeshes.size()))
                outSpec.MaterialOverrides[slotIndex] = AssetID(rawID);
        }

        if(reader.Failed)
        {
            Log<Severity::Error>("[MeshSerializer] {} is truncated or corrupted!", path);
            return false;
        }
        return true;
    }
}