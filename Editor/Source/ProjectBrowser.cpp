// Copyright (c) - SurgeTechnologies - All rights reserved
#include "ProjectBrowser.hpp"
#include "Surge/Utility/FileDialogs.hpp"
#include "Surge/Serializer/Serializer.hpp"
#include "Surge/Utility/Filesystem.hpp"
#include "Surge/Project/Project.hpp"
#include "Surge/ECS/Scene.hpp"
#include "Panels/SceneHierarchyPanel.hpp"
#include "Utility/ImGuiAux.hpp"
#include "Editor.hpp"
#include <imgui_stdlib.h>
#include <json/json.hpp>

#define PROJECT_DATA_FILENAME "Projects.json"
namespace Surge
{
    ProjectBrowserWindow::ProjectBrowserWindow()
    {
        mPersistantStoragePath = PlatformMisc::GetPersistantStoragePath();
        Filesystem::CreateOrEnsureFile(fmt::format("{0}/{1}", mPersistantStoragePath, PROJECT_DATA_FILENAME));
        LoadProjectsFromPersistantStorage();
    }

    void ProjectBrowserWindow::Render()
    {
        ImGui::Begin("Project Browser");
        {
            ImGuiAux::ScopedBoldFont scopedBoldFont(true);
            ImGuiAux::TextCentered("Welcome to Surge!");
            ImGui::Separator();
            ImGuiAux::TextCentered("Projects");
        }
        if (mPersistantProjectData.empty())
        {
            ImGuiAux::TextCentered("No projects found! Create a New Project to show them up here!");
        }
        else
        {
            if (ImGui::BeginTable("ProjectsTable", 3, ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable, {0.0f, 0.0f}, 0.0f))
            {
                ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
                ImGui::TableSetupColumn("Name");
                ImGui::TableSetupColumn("Path");
                ImGui::TableSetupColumn("Action");
                ImGui::TableHeadersRow();

                Uint index = 0;
                for (const PersistantProjectData& proj : mPersistantProjectData)
                {
                    ImGui::PushID(index);
                    constexpr Uint offset = 10;

                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(proj.Name.c_str());

                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(proj.AbsolutePath.c_str());

                    ImGui::TableNextColumn();
                    if (ImGuiAux::Button("Edit"))
                        LaunchProject(proj.Name, "Test", proj.AbsolutePath);
                    ImGui::SameLine();
                    if (ImGuiAux::Button("Remove"))
                        RemoveProjectFromPersistantStorage(index);

                    ImGui::PopID();
                    index++;
                }
                ImGui::EndTable();
                ImGui::PopFont();
            }
        }
        {
            ImGuiAux::ScopedBoldFont scopedBoldFont(true);
            ImGui::Separator();

            const char* conjustedTitle = "New Project Add Existing";
            float windowWidth = ImGui::GetWindowSize().x;
            float textWidth = ImGui::CalcTextSize(conjustedTitle).x;
            ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);

            if (ImGuiAux::Button("New Project"))
                ImGui::OpenPopup("New Project");
            ImGui::SameLine();
            if (ImGuiAux::Button("Add Existing"))
            {
            }
        }

        if (ImGui::BeginPopupModal("New Project"))
        {
            if (ImGui::BeginTable("ProjTable", 2, ImGuiTableFlags_Resizable))
            {
                ImGui::TableNextColumn();
                ImGui::TextUnformatted("Project Name");
                ImGui::TableNextColumn();
                ImGui::PushItemWidth(-1);
                ImGui::InputText("##Project Name", &mProjectNameBuffer);
                ImGui::PopItemWidth();
                !mProjectNameBuffer.empty() ? ImGuiAux::DrawRectAroundWidget(ImGuiAux::Colors::ThemeColor) : ImGuiAux::DrawRectAroundWidget(ImGuiAux::Colors::Red);

                ImGui::TableNextColumn();
                ImGui::TextUnformatted("Scene Name");
                ImGui::TableNextColumn();
                ImGui::PushItemWidth(-1);
                ImGui::InputText("##Scene Name", &mSceneNameBuffer);
                ImGui::PopItemWidth();
                !mSceneNameBuffer.empty() ? ImGuiAux::DrawRectAroundWidget(ImGuiAux::Colors::ThemeColor) : ImGuiAux::DrawRectAroundWidget(ImGuiAux::Colors::Red);

                ImGui::TableNextColumn();
                ImGui::TextUnformatted("Project Path");
                ImGui::TableNextColumn();
                ImGui::InputTextWithHint("##projPath", "C:/Dev/SurgeProject", &mProjectPathBuffer);
                Filesystem::Exists(mProjectPathBuffer) ? ImGuiAux::DrawRectAroundWidget(ImGuiAux::Colors::ThemeColor) : ImGuiAux::DrawRectAroundWidget(ImGuiAux::Colors::Red);
                ImGui::SameLine();
                if (ImGui::SmallButton("Choose Folder"))
                {
                    mProjectPathBuffer = FileDialog::ChooseFolder();
                    if (!mProjectPathBuffer.empty())
                        std::replace(mProjectPathBuffer.begin(), mProjectPathBuffer.end(), '\\', '/');
                }

                ImGui::EndTable();
            }

            if (!mProjectPathBuffer.empty() && Filesystem::Exists(mProjectPathBuffer))
            {
                ImGui::Separator();
                if (ImGuiAux::ButtonCentered("Yessir, Create the Project!"))
                {
                    SerializeProject();
                    LoadProjectsFromPersistantStorage();

                    ImGui::CloseCurrentPopup();
                    mProjectNameBuffer.clear();
                    mProjectPathBuffer.clear();
                    mSceneNameBuffer.clear();
                }
            }
            constexpr char* buttonText = "Nah, next time";
            int offset = 15;
            auto& windowSize = ImGui::GetWindowSize();
            auto& textSize = ImGui::CalcTextSize(buttonText);
            ImGui::SetCursorPos({windowSize.x - textSize.x - offset, windowSize.y - textSize.y - offset});
            if (ImGuiAux::Button(buttonText))
                ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }

        ImGui::End();
    }

    void ProjectBrowserWindow::LaunchProject(const String& name, const String& sceneName, const Path& path)
    {
        Editor* editor = static_cast<Editor*>(Surge::Core::GetClient());
        editor->SetActiveProject(new Project(name, path));

        Ref<Scene> scene = editor->mActiveProject->AddScene(sceneName);
        Serializer::Deserialize<Scene>("Engine/Assets/Scenes/Default.surge", scene.Raw());
        editor->mActiveProject->AddActiveSceneChangeCallback([&](Ref<Scene>& scene) {
            editor->mPanelManager.GetPanel<SceneHierarchyPanel>()->SetSceneContext(scene.Raw());
            editor->mRenderer->SetSceneContext(scene);
        });
        editor->mActiveProject->SetActiveScene(0);
    }

    void ProjectBrowserWindow::SerializeProject()
    {
        Path projectsPersistantDataPath = fmt::format("{0}/{1}", mPersistantStoragePath, PROJECT_DATA_FILENAME);
        String jsonContents = Filesystem::ReadFile<String>(projectsPersistantDataPath);
        nlohmann::json parsedJson = jsonContents.empty() ? nlohmann::json() : nlohmann::json::parse(jsonContents);

        Uint size;
        parsedJson.contains("Size") ? size = parsedJson["Size"] : size = 0;

        parsedJson[fmt::format("Project{0}", size)]["Path"] = mProjectPathBuffer;
        parsedJson[fmt::format("Project{0}", size)]["Name"] = mProjectNameBuffer;
        parsedJson["Size"] = ++size;

        String result = parsedJson.dump(4);
        FILE* f = nullptr;
        fopen_s(&f, projectsPersistantDataPath.c_str(), "w");
        if (f)
        {
            fwrite(result.c_str(), sizeof(char), result.size(), f);
            fclose(f);
        }
    }

    // Objective of this function is to Load and "Clean" the persistant json file
    void ProjectBrowserWindow::LoadProjectsFromPersistantStorage()
    {
        Vector<PersistantProjectData> result;
        bool writeFile = false;

        Path projectsPersistantDataPath = fmt::format("{0}/{1}", mPersistantStoragePath, PROJECT_DATA_FILENAME);
        String jsonContents = Filesystem::ReadFile<String>(projectsPersistantDataPath);
        nlohmann::json parsedJson = jsonContents.empty() ? nlohmann::json() : nlohmann::json::parse(jsonContents);

        Uint size;
        parsedJson.contains("Size") ? size = parsedJson["Size"] : size = 0;

        for (Uint i = 0; i < size; i++)
        {
            String currentProjectFmt = fmt::format("Project{0}", i);
            String path = parsedJson[currentProjectFmt]["Path"];
            String name = parsedJson[currentProjectFmt]["Name"];
            if (Filesystem::Exists(path)) // TODO: Add more checks to make sure the project is valid, curretly only checks for the valid path
            {
                PersistantProjectData& projData = result.emplace_back();
                projData.Name = name;
                projData.AbsolutePath = path;
            }
            else
            {
                // The path doesn't exist anymore, so remove the project from the json
                parsedJson.erase(currentProjectFmt);
                size--;
                writeFile = true;
            }
        }
        if (writeFile)
        {
            parsedJson["Size"] = size;

            String r = parsedJson.dump(4);
            FILE* f = nullptr;
            fopen_s(&f, projectsPersistantDataPath.c_str(), "w");
            if (f)
            {
                fwrite(r.c_str(), sizeof(char), r.size(), f);
                fclose(f);
            }
        }
        mPersistantProjectData = result;
    }

    void ProjectBrowserWindow::RemoveProjectFromPersistantStorage(Uint index)
    {
        mPersistantProjectData.erase(mPersistantProjectData.begin() + index);
        // TODO: Serialize
    }

} // namespace Surge
