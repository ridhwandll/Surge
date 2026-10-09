// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Settings/EditorSettings.hpp"
#include "Export/ExportSettingsIO.hpp"
#include "Surge/Core/Logger/Logger.hpp"
#include <json/json.hpp>
#include <array>

namespace Surge
{
    String EditorSettings::ToString() const
    {
        nlohmann::json j;
        j["ShowAxes"] = ShowAxes;
        j["AspectMode"] = AspectMode;

        j["GizmoOperation"] = GizmoOperation;
        j["GizmoWorldSpace"] = GizmoWorldSpace;
        j["SnapEnabled"] = SnapEnabled;
        j["SnapTranslate"] = SnapTranslate;
        j["SnapRotate"] = SnapRotate;
        j["SnapScale"] = SnapScale;

        j["HasCameraView"] = HasCameraView;
        j["CameraFocalPoint"] = { CameraFocalPoint.x, CameraFocalPoint.y, CameraFocalPoint.z };
        j["CameraDistance"] = CameraDistance;
        j["CameraPitch"] = CameraPitch;
        j["CameraYaw"] = CameraYaw;
        j["CameraSpeed"] = CameraSpeed;

        nlohmann::json& browser = j["ContentBrowser"];
        browser["ThumbnailSize"] = ContentBrowser.ThumbnailSize;
        browser["ListView"] = ContentBrowser.ListView;
        browser["SortField"] = ContentBrowser.SortField;
        browser["SortDescending"] = ContentBrowser.SortDescending;
        browser["FolderTreeWidth"] = ContentBrowser.FolderTreeWidth;
        browser["ShowUnimported"] = ContentBrowser.ShowUnimported;
        browser["SearchAllFolders"] = ContentBrowser.SearchAllFolders;
        browser["HiddenTypes"] = ContentBrowser.HiddenTypes;
        browser["LastDirectory"] = ContentBrowser.LastDirectory;
        browser["Favorites"] = ContentBrowser.Favorites;

        j["LastScene"] = LastScene.Get();
        j["FrameCap"] = FrameCap;
        return j.dump(4);
    }

    void EditorSettings::FromString(const String& text)
    {
        const nlohmann::json j = nlohmann::json::parse(text, nullptr, false);
        if(j.is_discarded() || !j.is_object())
            return;

        using ExportSettingsIO::Read;
        Read(j, "ShowAxes", ShowAxes);
        Read(j, "AspectMode", AspectMode);

        Read(j, "GizmoOperation", GizmoOperation);
        Read(j, "GizmoWorldSpace", GizmoWorldSpace);
        Read(j, "SnapEnabled", SnapEnabled);
        Read(j, "SnapTranslate", SnapTranslate);
        Read(j, "SnapRotate", SnapRotate);
        Read(j, "SnapScale", SnapScale);

        Read(j, "HasCameraView", HasCameraView);
        std::array<float, 3> focalPoint = { CameraFocalPoint.x, CameraFocalPoint.y, CameraFocalPoint.z };
        Read(j, "CameraFocalPoint", focalPoint);
        CameraFocalPoint = { focalPoint[0], focalPoint[1], focalPoint[2] };
        Read(j, "CameraDistance", CameraDistance);
        Read(j, "CameraPitch", CameraPitch);
        Read(j, "CameraYaw", CameraYaw);
        Read(j, "CameraSpeed", CameraSpeed);

        if(const auto it = j.find("ContentBrowser"); it != j.end())
        {
            const nlohmann::json& browser = *it;
            Read(browser, "ThumbnailSize", ContentBrowser.ThumbnailSize);
            Read(browser, "ListView", ContentBrowser.ListView);
            Read(browser, "SortField", ContentBrowser.SortField);
            Read(browser, "SortDescending", ContentBrowser.SortDescending);
            Read(browser, "FolderTreeWidth", ContentBrowser.FolderTreeWidth);
            Read(browser, "ShowUnimported", ContentBrowser.ShowUnimported);
            Read(browser, "SearchAllFolders", ContentBrowser.SearchAllFolders);
            Read(browser, "HiddenTypes", ContentBrowser.HiddenTypes);
            Read(browser, "LastDirectory", ContentBrowser.LastDirectory);
            Read(browser, "Favorites", ContentBrowser.Favorites);
        }

        uint64_t lastScene = LastScene.Get();
        Read(j, "LastScene", lastScene);
        LastScene = UUID(lastScene);
        Read(j, "FrameCap", FrameCap);

        // Hand edited files must not break the editor
        AspectMode = glm::clamp(AspectMode, 0, 4);
        SnapTranslate = glm::max(SnapTranslate, 0.001f);
        SnapRotate = glm::max(SnapRotate, 0.01f);
        SnapScale = glm::max(SnapScale, 0.001f);
        FrameCap = glm::clamp(FrameCap, 0.0f, 1000.0f);
        ContentBrowser.ThumbnailSize = glm::clamp(ContentBrowser.ThumbnailSize, 48.0f, 256.0f);
        ContentBrowser.SortField = glm::clamp(ContentBrowser.SortField, 0, 3);
        ContentBrowser.FolderTreeWidth = glm::clamp(ContentBrowser.FolderTreeWidth, 120.0f, 800.0f);
    }

    void EditorSettingsStore::Load(const Path& projectDir, EditorSettings& outSettings)
    {
        outSettings = {};
        mProjectDir = projectDir;
        mLastSaved.clear();

        String text;
        if(Filesystem::Exists(projectDir / EditorSettings::FILE_NAME) && Filesystem::ReadTextFile(projectDir / EditorSettings::FILE_NAME, text))
        {
            outSettings.FromString(text);
            mLastSaved = outSettings.ToString();
        }
    }

    void EditorSettingsStore::Save(const EditorSettings& settings)
    {
        if(mProjectDir.empty())
            return;

        String text = settings.ToString();
        if(text == mLastSaved)
            return; // Don't touch the file (or its VCS status) when nothing changed

        if(Filesystem::WriteTextFile(mProjectDir / EditorSettings::FILE_NAME, text))
            mLastSaved = std::move(text);
        else
            Log<Severity::Warn>("[Editor] Could not write {}", (mProjectDir / EditorSettings::FILE_NAME).string());
    }

} // namespace Surge
