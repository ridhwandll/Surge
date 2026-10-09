// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Defines.hpp"
#include "Surge/Core/Path.hpp"
#include "Surge/Core/String.hpp"
#include "Surge/Core/Vector.hpp"
#include "Surge/Asset/Asset.hpp"

namespace Surge
{
    // File operations inside the project's Assets folder that keep the AssetRegistry in sync: registered files (and every registered file
    // below a folder) keep their AssetID when renamed or moved, copies are imported with new IDs, deletes unregister and drop the cooked data.
    // Every function returns false and fills outError when nothing was done
    namespace AssetFileOps
    {
        Path GetAssetsRoot();
        String ToRelative(const Path& absolutePath); // Generic separators, relative to the Assets folder
        bool IsInside(const Path& path, const Path& folder); // path == folder or anywhere below it
        bool IsHidden(const Path& path);                     // Engine files (registry, cooked data) and dot files, never shown
        AssetType GetTypeFromExtension(const Path& path);    // NONE for files the engine can't import
        bool IsValidName(const String& name, String& outError);
        Path MakeUnique(const Path& desiredPath);            // "Name.ext" -> "Name (1).ext" while it exists

        // The registered assets at or below the path
        Vector<AssetID> CollectAssets(const Path& path);

        bool CreateFolder(const Path& parentFolder, const String& name, Path& outPath, String& outError);
        bool Rename(const Path& path, const String& newName, Path& outPath, String& outError);
        bool Move(const Path& path, const Path& destinationFolder, Path& outPath, String& outError);
        bool Copy(const Path& source, const Path& destinationFolder, Path& outPath, String& outError); // Source may be outside the project
        bool Delete(const Path& path, String& outError); // To the recycle bin

        // Registers a single file (INVALID for unknown extensions), or every importable file below a folder (returns how many were new)
        AssetID Import(const Path& file);
        Uint ImportFolder(const Path& folder);
    } // namespace AssetFileOps

} // namespace Surge
