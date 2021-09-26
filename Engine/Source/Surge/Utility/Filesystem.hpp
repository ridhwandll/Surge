// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once

namespace Surge::Filesystem
{
    String ReadFile(const Path& path);
    String RemoveExtension(const Path& path);
    String GetNameWithExtension(const Path& assetFilepath);
    String GetNameWithoutExtension(const Path& assetFilepath);
    bool Exists(const Path& path);
}
