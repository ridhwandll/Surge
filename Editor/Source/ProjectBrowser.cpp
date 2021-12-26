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
#include <fmt/chrono.h>
#include <IconsFontAwesome.hpp>

#define PROJECT_DATA_FILENAME "Projects.json"
namespace Surge
{
    void ProjectBrowserWindow::Initialize()
    {
        mPersistantStoragePath = PlatformMisc::GetPersistantStoragePath();
        Filesystem::CreateOrEnsureFile(fmt::format("{0}/{1}", mPersistantStoragePath, PROJECT_DATA_FILENAME));
        LoadProjectsFromPersistantStorage();
        for (const PersistantProjectData& proj : mPersistantProjectData)
        {
            if (proj.StartupProject)
                LaunchProject(proj);
        }
    }

    void ProjectBrowserWindow::Shutdown()
    {
        mPersistantProjectData.clear();
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
            if (ImGui::BeginTable("ProjectsTable", 3, ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_PadOuterX, {0.0f, 0.0f}, 0.0f))
            {
                ImGui::TableSetupColumn("NAME");
                ImGui::TableSetupColumn("INFO");
                ImGui::TableSetupColumn("ACTION");
                ImGui::TableHeadersRow();

                Uint index = 0;
                for (PersistantProjectData& proj : mPersistantProjectData)
                {
                    constexpr float shiftY = 10.0f;
                    ImGui::PushID(index);
                    ImGui::TableNextRow(0, 50.0f);

                    {
                        ImGuiAux::ScopedColor color = ImGuiAux::ScopedColor({ImGuiCol_Text}, proj.Valid ? ImGuiAux::Colors::Silver : ImGuiAux::Colors::Red);
                        ImGui::TableNextColumn();
                        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[2]);
                        ImGui::TextUnformatted(proj.Name.c_str());
                        ImGui::PopFont();
                        ImGui::TextUnformatted(proj.AbsolutePath.c_str());

                        if (proj.StartupProject)
                        {
                            ImGuiAux::ShiftCursorX(300);
                            ImGuiAux::ShiftCursorY(-25);
                            ImGui::TextUnformatted(ICON_SURGE_ARROW_LEFT);
                        }

                        ImGui::TableNextColumn();
                        ImGuiAux::ShiftCursorY(shiftY);
                        ImGui::Text("Creation date: %s", proj.CreationDateString.c_str());

                        ImGui::TableNextColumn();
                        if (proj.Valid)
                        {
                            ImGuiAux::ShiftCursorY(shiftY);
                            if (ImGuiAux::Button("Edit"))
                            {
                                LaunchProject(proj);
                                Core::GetWindow()->Maximize();
                            }
                            ImGui::SameLine();

                            if (!proj.StartupProject)
                            {
                                if (ImGuiAux::Button("Set as Startup Project"))
                                {
                                    for (PersistantProjectData& p : mPersistantProjectData)
                                        p.StartupProject = false;

                                    proj.StartupProject = true;
                                }
                                WriteProjectsToPersistantStorage();
                            }
                        }
                        else
                        {
                            if (ImGuiAux::Button("On no! Something went wrong with this project! \nClick this button to remove this from the list :\"(!"))
                                RemoveProjectFromPersistantStorage(index);
                        }
                    }

                    ImGui::PopID();
                    index++;
                }
                ImGui::EndTable();
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
                mProjectPathBuffer = FileDialog::OpenFile("SurgeProjectFile (*.surgeProj)\0*.surgeProj\0*");
                if (!mProjectPathBuffer.empty())
                {
                    const auto dateTimeNow = std::chrono::system_clock::now();

                    ProjectMetadata metadata;
                    Serializer::Deserialize<ProjectMetadata>(mProjectPathBuffer, &metadata);

                    PersistantProjectData& pp = mPersistantProjectData.emplace_back();
                    pp.AbsolutePath = metadata.ProjPath;
                    pp.Name = metadata.Name;
                    pp.CreationDateString = fmt::format("{:%d/%m/%Y}", dateTimeNow);

                    WriteProjectsToPersistantStorage();
                    LoadProjectsFromPersistantStorage();
                }
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
                    const auto dateTimeNow = std::chrono::system_clock::now();

                    // Temporary project to generate the metadata and write it the file [Do not remove]
                    ProjectMetadata temp = ProjectMetadata(mProjectNameBuffer, mProjectPathBuffer);

                    PersistantProjectData& pp = mPersistantProjectData.emplace_back();
                    pp.AbsolutePath = mProjectPathBuffer;
                    pp.Name = mProjectNameBuffer;
                    pp.CreationDateString = fmt::format("{:%d/%m/%Y}", dateTimeNow);
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

    void ProjectBrowserWindow::LaunchProject(const PersistantProjectData& projData)
    {
        ProjectMetadata metadata;
        String metadataPath = fmt::format("{0}/.surge/{1}.surgeProj", projData.AbsolutePath, projData.Name);
        Serializer::Deserialize<ProjectMetadata>(metadataPath, &metadata);

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
            j[uuidStr]["CreationDate"] = projData.CreationDateString;
            j[uuidStr]["StartupProject"] = projData.StartupProject;
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
        mPersistantProjectData.clear();

        Path projectsPersistantDataPath = fmt::format("{0}/{1}", mPersistantStoragePath, PROJECT_DATA_FILENAME);
        String jsonContents = Filesystem::ReadFile<String>(projectsPersistantDataPath);
        const nlohmann::json parsedJson = jsonContents.empty() ? nlohmann::json() : nlohmann::json::parse(jsonContents);

        for (const nlohmann::json& j : parsedJson)
        {
            PersistantProjectData& projData = mPersistantProjectData.emplace_back();
            projData.Name = j["Name"];
            projData.CreationDateString = j["CreationDate"];
            projData.AbsolutePath = j["Path"];
            projData.StartupProject = j["StartupProject"];

            // TODO: Add more checks to make sure the project is valid, curretly only checks for the valid path
            if (Filesystem::Exists(projData.AbsolutePath) && Filesystem::Exists(fmt::format("{0}/.surge/{1}.surgeProj", projData.AbsolutePath, projData.Name)))
                projData.Valid = true;
            else
                projData.Valid = false;
        }
    }

    void ProjectBrowserWindow::RemoveProjectFromPersistantStorage(Uint index)
    {
        mPersistantProjectData.erase(mPersistantProjectData.begin() + index);
        WriteProjectsToPersistantStorage();
    }

} // namespace Surge
