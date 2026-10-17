// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/Shader/ShaderBinaryFormat.hpp"
#include "Surge/Asset/Serializer/BinaryHelpers.hpp"
#include "Surge/Utility/Filesystem.hpp"

namespace Surge::ShaderBinary
{
    static constexpr Uint SPIRV_MAGIC = 0x07230203;
    static constexpr Uint MAX_INCLUDES = 256;

    static void HashBytes(uint64_t& hash, const Byte* data, size_t size) // FNV-1a, '\r' skipped so CRLF checkouts hash like LF ones
    {
        for (size_t i = 0; i < size; i++)
        {
            if (data[i] == '\r')
                continue;
            hash ^= data[i];
            hash *= 1099511628211ull;
        }
    }

    static bool ReadPrefix(BinaryReader& reader, Header& outHeader, Vector<String>& outIncludes)
    {
        reader.Read(outHeader);
        if (reader.Failed || outHeader.Magic != MAGIC || outHeader.Version != VERSION || outHeader.IncludeCount > MAX_INCLUDES)
            return false;

        outIncludes.resize(outHeader.IncludeCount);
        for (String& include : outIncludes)
            include = reader.ReadStr();
        return !reader.Failed;
    }

    String GetCookedPath(const String& sourcePath)
    {
        return std::format("{0}/Cooked/{1}.rSHADER", Filesystem::GetParentPath(sourcePath).string(), Filesystem::GetFilenameWithoutExt(sourcePath));
    }

    uint64_t HashSource(const String& sourcePath, const Vector<String>& includes)
    {
        uint64_t hash = 14695981039346656037ull;
        Vector<Byte> content;
        if (!Filesystem::ReadBinaryFile(sourcePath, content))
            return 0;
        HashBytes(hash, content.data(), content.size());

        const String baseDirectory = Filesystem::GetParentPath(sourcePath).string();
        for (const String& include : includes)
        {
            HashBytes(hash, reinterpret_cast<const Byte*>(include.data()), include.size());
            if (!Filesystem::ReadBinaryFile(std::format("{0}/{1}", baseDirectory, include), content))
                return 0;
            HashBytes(hash, content.data(), content.size());
        }
        return hash == 0 ? 1 : hash; // 0 means unreadable
    }

    bool Write(const String& path, const CookedShader& shader)
    {
        Header header;
        header.SourceHash = shader.SourceHash;
        header.Stages = static_cast<Uint>(shader.Stages);
        header.IncludeCount = static_cast<Uint>(shader.Includes.size());

        Vector<Byte> buffer;
        WriteData(buffer, header);
        for (const String& include : shader.Includes)
            WriteStr(buffer, include);

        WriteData(buffer, static_cast<Uint>(shader.SPIRVs.size()));
        for (const SPIRVHandle& spirv : shader.SPIRVs)
        {
            WriteData(buffer, static_cast<Uint>(spirv.Type));
            WriteData(buffer, static_cast<Uint>(spirv.SPIRV.size()));
            WriteDataArray(buffer, spirv.SPIRV.data(), spirv.SPIRV.size());
        }
        shader.Reflection.Serialize(buffer);

        Filesystem::CreateOrEnsureDirectories(Filesystem::GetParentPath(path));
        if (!Filesystem::WriteBinaryFile(path, buffer.data(), buffer.size()))
        {
            Log<Severity::Error>("[ShaderBinary] Failed to write: {}", path);
            return false;
        }
        return true;
    }

    bool Read(const String& path, CookedShader& outShader)
    {
        outShader = CookedShader();

        Vector<Byte> buffer;
        if (!Filesystem::ReadBinaryFile(path, buffer))
        {
            Log<Severity::Error>("[ShaderBinary] Failed to read: {}", path);
            return false;
        }

        BinaryReader reader(buffer.data(), buffer.size());
        Header header;
        if (!ReadPrefix(reader, header, outShader.Includes))
        {
            Log<Severity::Error>("[ShaderBinary] Outdated or corrupted cooked shader (expected version {}): {}", VERSION, path);
            return false;
        }
        outShader.SourceHash = header.SourceHash;
        outShader.Stages = static_cast<ShaderType>(header.Stages);

        Uint stageCount = 0;
        reader.Read(stageCount);
        if (!reader.Fits(stageCount, sizeof(Uint) * 2))
        {
            Log<Severity::Error>("[ShaderBinary] Corrupted cooked shader (stage count): {}", path);
            return false;
        }

        ShaderType stagesFound = ShaderType::NONE;
        outShader.SPIRVs.resize(stageCount);
        for (SPIRVHandle& spirv : outShader.SPIRVs)
        {
            Uint type = 0;
            Uint wordCount = 0;
            reader.Read(type);
            reader.Read(wordCount);
            if (reader.Failed || wordCount == 0 || !reader.Fits(wordCount, sizeof(Uint)))
            {
                Log<Severity::Error>("[ShaderBinary] Corrupted cooked shader (SPIR-V size): {}", path);
                return false;
            }

            spirv.Type = static_cast<ShaderType>(type);
            spirv.SPIRV.resize(wordCount);
            reader.ReadArray(spirv.SPIRV.data(), wordCount);
            if (spirv.SPIRV[0] != SPIRV_MAGIC || (static_cast<Uint>(spirv.Type) & static_cast<Uint>(outShader.Stages)) == 0)
            {
                Log<Severity::Error>("[ShaderBinary] Corrupted cooked shader (invalid stage): {}", path);
                return false;
            }
            stagesFound |= spirv.Type;
        }

        if (stagesFound != outShader.Stages || !outShader.Reflection.Deserialize(reader))
        {
            Log<Severity::Error>("[ShaderBinary] Corrupted cooked shader (reflection): {}", path);
            return false;
        }
        return true;
    }

    bool ReadSourceInfo(const String& path, uint64_t& outHash, Vector<String>& outIncludes)
    {
        Vector<Byte> buffer;
        if (!Filesystem::ReadBinaryFile(path, buffer))
            return false;

        BinaryReader reader(buffer.data(), buffer.size());
        Header header;
        if (!ReadPrefix(reader, header, outIncludes))
            return false;

        outHash = header.SourceHash;
        return true;
    }
}
