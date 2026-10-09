// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Asset/AssetFileOps.hpp"
#include "Asset/SourceWriters/MaterialSourceWriter.hpp"
#include "Surge/Asset/AssetManager.hpp"
#include "Surge/Core/Core.hpp"
#include "Surge/Utility/Filesystem.hpp"
#include "Surge/Utility/Platform.hpp"

#include <algorithm>
#include <cctype>

namespace Surge::AssetFileOps
{
    namespace fs = std::filesystem;

    static String ToLower(String text)
    {
        std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return text;
    }

    // Registry paths of everything at/below oldRelative move to newRelative, the registry is written once
    static void RebaseAssets(const String& oldRelative, const String& newRelative)
    {
        AssetManager* assetManager = Core::GetAssetManager();
        Vector<Pair<AssetID, String>> updates;
        for(const auto& [id, meta] : assetManager->GetRegistryMap())
        {
            if(HasFlag(meta.Flags, AssetFlags::MEMORY))
                continue;

            if(meta.RelativePath == oldRelative)
                updates.push_back({ id, newRelative });
            else if(meta.RelativePath.size() > oldRelative.size() && meta.RelativePath.starts_with(oldRelative) && meta.RelativePath[oldRelative.size()] == '/')
                updates.push_back({ id, newRelative + meta.RelativePath.substr(oldRelative.size()) });
        }

        for(const auto& [id, relativePath] : updates)
            assetManager->UpdateAssetPath(id, relativePath, false);
        if(!updates.empty())
            assetManager->SerializeRegistry();
    }

    static AssetID ImportWithoutSerialize(const Path& file)
    {
        if(IsHidden(file) || fs::is_directory(file))
            return AssetID::INVALID;

        const AssetType type = GetTypeFromExtension(file);
        if(type == AssetType::NONE)
            return AssetID::INVALID;

        return Core::GetAssetManager()->Import(ToRelative(file), type);
    }

    Path GetAssetsRoot()
    {
        Path root = Path(Core::GetAssetManager()->GetAssetsDirectory()).lexically_normal();
        if(!root.has_filename() && root.has_parent_path())
            root = root.parent_path(); // "Assets/" -> "Assets", so it compares equal to its children's parent_path()
        return root;
    }

    String ToRelative(const Path& absolutePath)
    {
        const Path root = GetAssetsRoot();
        Path relative = absolutePath.lexically_normal().lexically_relative(root);
        if(relative.empty() || *relative.begin() == "..")
        {
            std::error_code error;
            relative = fs::relative(absolutePath, root, error); // Mixed absolute/relative spellings
        }
        const String result = relative.generic_string();
        return result == "." ? String() : result;
    }

    bool IsInside(const Path& path, const Path& folder)
    {
        const Path relative = path.lexically_normal().lexically_relative(folder.lexically_normal());
        return !relative.empty() && *relative.begin() != "..";
    }

    bool IsHidden(const Path& path)
    {
        const String name = path.filename().string();
        if(name.empty() || name[0] == '.')
            return true;

        // Only the root level ones belong to the engine
        if(path.lexically_normal().parent_path() != GetAssetsRoot())
            return false;
        return name == "Internal" || name == "AssetRegistry.surge";
    }

    AssetType GetTypeFromExtension(const Path& path)
    {
        return AssetTypeFromExtension(ToLower(path.extension().string()).c_str());
    }

    bool IsValidName(const String& name, String& outError)
    {
        if(name.empty() || name == "." || name == "..")
        {
            outError = "The name can't be empty";
            return false;
        }
        if(name.find_first_of("<>:\"/\\|?*") != String::npos || std::any_of(name.begin(), name.end(), [](unsigned char c) { return c < 32; }))
        {
            outError = "A name can't contain any of < > : \" / \\ | ? *";
            return false;
        }
        if(name.back() == '.' || name.back() == ' ')
        {
            outError = "A name can't end with a dot or a space";
            return false;
        }

        static constexpr const char* reserved[] = { "con", "prn", "aux", "nul", "com1", "com2", "com3", "com4", "com5", "com6", "com7", "com8", "com9",
                                                    "lpt1", "lpt2", "lpt3", "lpt4", "lpt5", "lpt6", "lpt7", "lpt8", "lpt9" };
        const String stem = ToLower(Path(name).stem().string());
        if(std::find(std::begin(reserved), std::end(reserved), stem) != std::end(reserved))
        {
            outError = std::format("'{}' is a reserved name on Windows", name);
            return false;
        }
        return true;
    }

    Path MakeUnique(const Path& desiredPath)
    {
        if(!fs::exists(desiredPath))
            return desiredPath;

        const Path folder = desiredPath.parent_path();
        const String stem = desiredPath.stem().string();
        const String extension = fs::is_directory(desiredPath) ? String() : desiredPath.extension().string();
        const String base = fs::is_directory(desiredPath) ? desiredPath.filename().string() : stem;
        for(int i = 1;; i++)
        {
            const Path candidate = folder / std::format("{} ({}){}", base, i, extension);
            if(!fs::exists(candidate))
                return candidate;
        }
    }

    Vector<AssetID> CollectAssets(const Path& path)
    {
        const String relative = ToRelative(path);
        Vector<AssetID> result;
        for(const auto& [id, meta] : Core::GetAssetManager()->GetRegistryMap())
        {
            if(HasFlag(meta.Flags, AssetFlags::MEMORY))
                continue;
            if(meta.RelativePath == relative || (meta.RelativePath.size() > relative.size() && meta.RelativePath.starts_with(relative) && meta.RelativePath[relative.size()] == '/'))
                result.push_back(id);
        }
        return result;
    }

    bool CreateFolder(const Path& parentFolder, const String& name, Path& outPath, String& outError)
    {
        if(!IsValidName(name, outError))
            return false;

        outPath = MakeUnique(parentFolder / name);
        std::error_code error;
        if(!fs::create_directory(outPath, error))
        {
            outError = std::format("Could not create the folder: {}", error.message());
            return false;
        }
        return true;
    }

    bool Rename(const Path& path, const String& newName, Path& outPath, String& outError)
    {
        if(!IsValidName(newName, outError))
            return false;

        outPath = path.parent_path() / newName;
        if(outPath == path)
            return true;

        // Only the letter case changes: the OS sees the same file
        const bool caseOnly = ToLower(outPath.generic_string()) == ToLower(path.generic_string());
        if(!caseOnly && fs::exists(outPath))
        {
            outError = std::format("'{}' already exists", newName);
            return false;
        }

        const String oldRelative = ToRelative(path);
        std::error_code error;
        fs::rename(path, outPath, error);
        if(error)
        {
            outError = std::format("Could not rename: {}", error.message());
            return false;
        }
        RebaseAssets(oldRelative, ToRelative(outPath));

        // A material's name comes from its file, scripts are recompiled from the new path
        AssetManager* assetManager = Core::GetAssetManager();
        if(!fs::is_directory(outPath))
        {
            const AssetID id = assetManager->GetIDFromPath(ToRelative(outPath));
            const AssetType type = id ? assetManager->GetMetadata(id).Type : AssetType::NONE;
            if(type == AssetType::MATERIAL)
            {
                if(Ref<Material> material = assetManager->Load<Material>(id))
                {
                    material->SetName(outPath.stem().string());
                    MaterialSourceWriter::Write(material, outPath.generic_string());
                    assetManager->Save(id);
                    assetManager->Unload(id);
                }
            }
            else if(type == AssetType::SCRIPT && assetManager->IsLoaded(id))
            {
                assetManager->Save(id);
                assetManager->Unload(id);
            }
        }
        return true;
    }

    bool Move(const Path& path, const Path& destinationFolder, Path& outPath, String& outError)
    {
        if(path.parent_path().lexically_normal() == destinationFolder.lexically_normal())
        {
            outPath = path;
            return true; // Already there
        }
        if(fs::is_directory(path) && IsInside(destinationFolder, path))
        {
            outError = std::format("Can't move '{}' into itself", path.filename().string());
            return false;
        }

        outPath = MakeUnique(destinationFolder / path.filename());
        const String oldRelative = ToRelative(path);
        std::error_code error;
        fs::rename(path, outPath, error);
        if(error)
        {
            outError = std::format("Could not move '{}': {}", path.filename().string(), error.message());
            return false;
        }
        RebaseAssets(oldRelative, ToRelative(outPath));
        return true;
    }

    bool Copy(const Path& source, const Path& destinationFolder, Path& outPath, String& outError)
    {
        const bool isDirectory = fs::is_directory(source);
        if(isDirectory && IsInside(destinationFolder, source))
        {
            outError = std::format("Can't copy '{}' into itself", source.filename().string());
            return false;
        }

        outPath = MakeUnique(destinationFolder / source.filename());
        std::error_code error;
        if(isDirectory)
            fs::copy(source, outPath, fs::copy_options::recursive, error);
        else
            fs::copy_file(source, outPath, error);
        if(error)
        {
            outError = std::format("Could not copy '{}': {}", source.filename().string(), error.message());
            return false;
        }

        // Copies are new assets
        if(isDirectory)
            ImportFolder(outPath);
        else
            Import(outPath);
        return true;
    }

    bool Delete(const Path& path, String& outError)
    {
        AssetManager* assetManager = Core::GetAssetManager();
        const Vector<AssetID> assets = CollectAssets(path);

        Vector<String> cookedFiles;
        cookedFiles.reserve(assets.size());
        for(AssetID id : assets)
            cookedFiles.push_back(assetManager->GetSidecarPath(id));

        if(!Platform::MoveToRecycleBin(path.string()))
        {
            outError = std::format("Could not move '{}' to the Recycle Bin, is it open somewhere?", path.filename().string());
            return false;
        }

        std::error_code error;
        for(const String& cooked : cookedFiles)
            fs::remove(cooked, error); // Generated data, recreated by a reimport

        for(AssetID id : assets)
        {
            assetManager->Unload(id); // Only succeeds when nothing references it anymore
            assetManager->UnregisterAsset(id);
        }
        return true;
    }

    AssetID Import(const Path& file)
    {
        const AssetID id = ImportWithoutSerialize(file);
        if(id)
            Core::GetAssetManager()->SerializeRegistry();
        return id;
    }

    Uint ImportFolder(const Path& folder)
    {
        AssetManager* assetManager = Core::GetAssetManager();
        const size_t registeredBefore = assetManager->GetRegistryMap().size();

        std::error_code error;
        for(auto it = fs::recursive_directory_iterator(folder, fs::directory_options::skip_permission_denied, error); !error && it != fs::recursive_directory_iterator(); it.increment(error))
        {
            if(IsHidden(it->path()))
            {
                if(it->is_directory())
                    it.disable_recursion_pending();
                continue;
            }
            if(it->is_regular_file())
                ImportWithoutSerialize(it->path());
        }

        const Uint imported = static_cast<Uint>(assetManager->GetRegistryMap().size() - registeredBefore);
        if(imported > 0)
            assetManager->SerializeRegistry();
        return imported;
    }

} // namespace Surge::AssetFileOps
