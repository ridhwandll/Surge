// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once

namespace Surge::Filesystem
{
    void CreateFile(const Path& path);

    // Returns empty String if failed to open the Path
    template <typename T>
    T ReadFile(const Path& path);

    String RemoveExtension(const Path& path);
    String GetNameWithExtension(const Path& assetFilepath);
    String GetNameWithoutExtension(const Path& assetFilepath);
    Path GetParentPath(const Path& path);

    bool Exists(const Path& path);

} // namespace Surge::Filesystem
