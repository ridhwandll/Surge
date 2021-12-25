// Copyright (c) - SurgeTechnologies - All rights reserved
#include "ProjectBrowser.hpp"
#include "Surge/Utility/FileDialogs.hpp"
#include "Surge/Serializer/Serializer.hpp"
#include "Surge/Utility/Filesystem.hpp"
#include "Surge/Project/Project.hpp"
#include "Panels/SceneHierarchyPanel.hpp"
#include "Utility/ImGuiAux.hpp"
#include <json/json.hpp>
#include <imgui_stdlib.h>

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
        ImGui::Begin("Project Browser", 0, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);
        {
            ImGuiAux::ScopedBoldFont scopedBoldFont(true);
            ImGuiAux::TextCentered("Projects");
            ImGui::Spacing();
        }
        if (mPersistantProjectData.empty())
        {
            ImGuiAux::TextCentered("No projects found :(");
            ImGuiAux::TextCentered("Create a New Project or Add Existing ones to access them from here!");
        }
        else
        {
            if (ImGui::BeginTable("ProjectsTable", 3, ImGuiTableFlags_Resizable | ImGuiTableFlags_NoBordersInBodyUntilResize, {0.0f, 0.0f}, 0.0f))
            {
                ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
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
                    {
                        ProjectMetadata metadata;
                        String metadataPath = fmt::format("{0}/.surge/{1}.surgeProj", proj.AbsolutePath, proj.Name);
                        Serializer::Deserialize<ProjectMetadata>(metadataPath, &metadata);
                        LaunchProject(metadata);
                        Core::GetWindow()->Maximize();
                    }
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

            //const char* conjustedTitle = "New Project Add Existing";
            const char* conjustedTitle = "New Project";
            float windowWidth = ImGui::GetWindowSize().x;
            float textWidth = ImGui::CalcTextSize(conjustedTitle).x;
            ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);

            if (ImGuiAux::Button("New Project"))
                ImGui::OpenPopup("New Project");
            //ImGui::SameLine();
            //if (ImGuiAux::Button("Add Existing"))
            //{
            //    mProjectPathBuffer = FileDialog::OpenFile("SurgeProjectFile (*.surgeProj)\0*.surgeProj\0*");
            //    if (!mProjectPathBuffer.empty())
            //    {
            //        ProjectMetadata metadata;
            //        Serializer::Deserialize<ProjectMetadata>(mProjectPathBuffer, &metadata);
            //        PersistantProjectData& pp = mPersistantProjectData.emplace_back();
            //        pp.AbsolutePath = mProjectPathBuffer;
            //        pp.Name = metadata.Name;
            //
            //        WriteProjectsToPersistantStorage();
            //        LoadProjectsFromPersistantStorage();
            //    }
            //}
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
                    Project temp;
                    temp.Invalidate(mProjectNameBuffer, mProjectPathBuffer);

                    PersistantProjectData& pp = mPersistantProjectData.emplace_back();
                    pp.AbsolutePath = mProjectPathBuffer;
                    pp.Name = mProjectNameBuffer;

                    WriteProjectsToPersistantStorage();
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

    void ProjectBrowserWindow::LaunchProject(const ProjectMetadata& metadata)
    {
        SG_ASSERT(mOnProjectLaunch, "Cannot launch project!");
        mOnProjectLaunch(metadata);
    }

    void ProjectBrowserWindow::WriteProjectsToPersistantStorage()
    {
        Path projectsPersistantDataPath = fmt::format("{0}/{1}", mPersistantStoragePath, PROJECT_DATA_FILENAME);
        nlohmann::json j = nlohmann::json();
        String result = j.dump(4);

        for (PersistantProjectData& projData : mPersistantProjectData)
        {
            String uuidStr = fmt::format("{0}", UUID());
            j[uuidStr]["Path"] = projData.AbsolutePath;
            j[uuidStr]["Name"] = projData.Name;
        }
        result = j.dump(4);
        FILE* f = nullptr;
        fopen_s(&f, projectsPersistantDataPath.c_str(), "w");
        if (f)
        {
            fwrite(result.c_str(), sizeof(char), result.size(), f);
            fclose(f);
        }
    }

    void ProjectBrowserWindow::LoadProjectsFromPersistantStorage()
    {
        Vector<PersistantProjectData> result;

        Path projectsPersistantDataPath = fmt::format("{0}/{1}", mPersistantStoragePath, PROJECT_DATA_FILENAME);
        String jsonContents = Filesystem::ReadFile<String>(projectsPersistantDataPath);
        const nlohmann::json parsedJson = jsonContents.empty() ? nlohmann::json() : nlohmann::json::parse(jsonContents);

        for (const nlohmann::json& j : parsedJson)
        {
            String path = j["Path"];
            String name = j["Name"];
            if (Filesystem::Exists(path)) // TODO: Add more checks to make sure the project is valid, curretly only checks for the valid path
            {
                PersistantProjectData& projData = result.emplace_back();
                projData.Name = name;
                projData.AbsolutePath = path;
            }
        }
        mPersistantProjectData = result;
    }

    void ProjectBrowserWindow::RemoveProjectFromPersistantStorage(Uint index)
    {
        mPersistantProjectData.erase(mPersistantProjectData.begin() + index);
        WriteProjectsToPersistantStorage();
    }

} // namespace Surge
