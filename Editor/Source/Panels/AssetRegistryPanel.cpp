// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Panels/AssetRegistryPanel.hpp"
#include "Panels/ContentBrowserPanel.hpp"
#include "Surge/Asset/AssetManager.hpp"
#include "Surge/Core/Core.hpp"
#include "SurgeReflect/Enum.hpp"
#include "Utility/ImGuiAux.hpp"
#include "Editor.hpp"

#include <algorithm>
#include <cctype>

namespace Surge
{
    enum RegistryColumn
    {
        COLUMN_ID = 0,
        COLUMN_TYPE,
        COLUMN_STATUS,
        COLUMN_PATH,
        COLUMN_ACTIONS
    };

    static String ToLower(String text)
    {
        std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return text;
    }

    // Sort key of the status column: missing first, then built-in, loaded, on disk
    static int StatusRank(const AssetMetadata& meta)
    {
        if(HasFlag(meta.Flags, AssetFlags::MISSING))
            return 0;
        if(HasFlag(meta.Flags, AssetFlags::MEMORY))
            return 1;
        if(HasFlag(meta.Flags, AssetFlags::LOADED))
            return 2;
        return 3;
    }

    void AssetRegistryPanel::Init(void*)
    {
        mCode = GetStaticCode();
        mAssetManager = Core::GetAssetManager();
    }

    void AssetRegistryPanel::Render(bool* show)
    {
        if(!*show)
            return;

        if(ImGui::Begin(PanelCodeToString(mCode), show))
        {
            DrawToolbar();
            ImGui::Dummy(ImVec2(0.0f, 4.0f));
            DrawTable();
        }
        ImGui::End();

        for(AssetID id : mToUnload)
            mAssetManager->Unload(id);
        for(AssetID id : mToUnregister)
            mAssetManager->UnregisterAsset(id);
        if(!mToUnload.empty() || !mToUnregister.empty())
            mSortDirty = true;
        mToUnload.clear();
        mToUnregister.clear();
    }

    void AssetRegistryPanel::DrawToolbar()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
        {
            ImGuiAux::ScopedBoldFont font;
            if(ImGuiAux::Button("SERIALIZE TO DISK"))
                mAssetManager->SerializeRegistry();
            ImGui::SameLine();

            if(ImGuiAux::Button("UNLOAD UNUSED"))
            {
                for(const auto& [id, meta] : mAssetManager->GetRegistryMap())
                {
                    // The AssetManager is the only one holding a reference
                    if(HasFlag(meta.Flags, AssetFlags::LOADED) && !HasFlag(meta.Flags, AssetFlags::MEMORY) && mAssetManager->GetAssetRefCount(id) <= 1)
                        mToUnload.push_back(id);
                }
            }
            ImGuiAux::ItemTooltip("Frees every loaded asset nothing references anymore");
        }

        ImGui::SameLine();
        ImGui::TextDisabled("%zu / %zu assets", mRows.size(), mAssetManager->GetRegistryMap().size());

        constexpr float searchWidth = 250.0f;
        constexpr float comboWidth = 140.0f;
        const float rightWidth = searchWidth + comboWidth + ImGui::GetStyle().ItemSpacing.x;
        ImGui::SameLine(std::max(ImGui::GetCursorPosX(), ImGui::GetWindowContentRegionMax().x - rightWidth));

        ImGui::SetNextItemWidth(comboWidth);
        const char* preview = mFilterType == AssetType::NONE ? "ALL TYPES" : SurgeReflect::EnumToString(mFilterType).data();
        if(ImGui::BeginCombo("##TypeFilter", preview))
        {
            if(ImGui::Selectable("ALL TYPES", mFilterType == AssetType::NONE))
                mFilterType = AssetType::NONE;
            for(AssetType type : sAssetTypeArray)
            {
                if(ImGui::Selectable(SurgeReflect::EnumToString(type).data(), mFilterType == type))
                    mFilterType = type;
            }
            ImGui::EndCombo();
        }

        ImGui::SameLine();
        ImGui::SetNextItemWidth(searchWidth);
        ImGui::InputTextWithHint("##RegistrySearch", "Search ID, type or path...", mSearchBuffer, IM_COUNTOF(mSearchBuffer));
        ImGui::PopStyleVar();
    }

    void AssetRegistryPanel::RebuildRows()
    {
        const String search = ToLower(mSearchBuffer);
        const auto& registry = mAssetManager->GetRegistryMap();

        mRows.clear();
        mRows.reserve(registry.size());
        for(const auto& [id, meta] : registry)
        {
            if(mFilterType != AssetType::NONE && meta.Type != mFilterType)
                continue;

            if(!search.empty())
            {
                const bool matches = std::to_string(id.Get()).find(search) != String::npos || ToLower(SurgeReflect::EnumToString(meta.Type).data()).find(search) != String::npos ||
                                     ToLower(meta.RelativePath).find(search) != String::npos;
                if(!matches)
                    continue;
            }
            mRows.push_back(id);
        }

        std::sort(mRows.begin(), mRows.end(), [this](AssetID a, AssetID b) {
            const AssetMetadata& ma = mAssetManager->GetMetadata(a);
            const AssetMetadata& mb = mAssetManager->GetMetadata(b);
            int order = 0;
            switch(mSortColumn)
            {
                case COLUMN_ID: order = a.Get() < b.Get() ? -1 : (a.Get() > b.Get() ? 1 : 0); break;
                case COLUMN_TYPE: order = static_cast<int>(ma.Type) - static_cast<int>(mb.Type); break;
                case COLUMN_STATUS: order = StatusRank(ma) - StatusRank(mb); break;
                default: break;
            }
            if(order == 0)
                order = ma.RelativePath.compare(mb.RelativePath);
            return mSortDescending ? order > 0 : order < 0;
        });

        mRowsSearch = mSearchBuffer;
        mRowsFilter = mFilterType;
        mRowsRegistrySize = registry.size();
        mSortDirty = false;
    }

    void AssetRegistryPanel::DrawTable()
    {
        constexpr ImGuiTableFlags flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_RowBg | ImGuiTableFlags_Sortable;
        if(!ImGui::BeginTable("RegistryTable", 5, flags))
            return;

        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("Asset ID", ImGuiTableColumnFlags_WidthFixed, 180.0f, COLUMN_ID);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 110.0f, COLUMN_TYPE);
        ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 80.0f, COLUMN_STATUS);
        ImGui::TableSetupColumn("Relative Path", ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_DefaultSort, 0.0f, COLUMN_PATH);
        ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort, 100.0f, COLUMN_ACTIONS);
        ImGui::TableHeadersRow();

        if(ImGuiTableSortSpecs* specs = ImGui::TableGetSortSpecs(); specs && specs->SpecsDirty && specs->SpecsCount > 0)
        {
            mSortColumn = static_cast<int>(specs->Specs[0].ColumnUserID);
            mSortDescending = specs->Specs[0].SortDirection == ImGuiSortDirection_Descending;
            specs->SpecsDirty = false;
            mSortDirty = true;
        }

        // Load states change without the registry changing, so a status sort refreshes twice a second
        static float sRefreshTimer = 0.0f;
        sRefreshTimer += ImGui::GetIO().DeltaTime;
        if(mSortDirty || mRowsSearch != mSearchBuffer || mRowsFilter != mFilterType || mRowsRegistrySize != mAssetManager->GetRegistryMap().size() || sRefreshTimer > 0.5f)
        {
            RebuildRows();
            sRefreshTimer = 0.0f;
        }

        Editor* editor = static_cast<Editor*>(Core::GetClient());
        ImFont* boldFont = ImGui::GetIO().Fonts->Fonts[1];

        ImGuiListClipper clipper;
        clipper.Begin(static_cast<int>(mRows.size()));
        while(clipper.Step())
        {
            for(int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++)
            {
                const AssetID id = mRows[row];
                const AssetMetadata& meta = mAssetManager->GetMetadata(id);
                if(!meta.ID)
                    continue; // Unregistered this frame

                ImGui::PushID(static_cast<int>(id.Get() ^ (id.Get() >> 32)));
                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(COLUMN_ID);
                const String idText = std::to_string(id.Get());
                ImGui::Selectable(idText.c_str(), false, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap);
                if(ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && !HasFlag(meta.Flags, AssetFlags::MEMORY))
                    editor->GetPanelManager().GetPanel<ContentBrowserPanel>()->SetSelectedAsset(id);

                if(ImGuiAux::BeginStyledPopupContextItem())
                {
                    {
                        ImGuiAux::ScopedBoldFont font;
                        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s", meta.RelativePath.c_str());
                    }
                    ImGuiAux::StyledSeparator();
                    if(ImGuiAux::StyledMenuItem("Reveal in Content Browser", "Double-click", nullptr, !HasFlag(meta.Flags, AssetFlags::MEMORY)))
                        editor->GetPanelManager().GetPanel<ContentBrowserPanel>()->SetSelectedAsset(id);
                    if(ImGuiAux::StyledMenuItem("Copy Asset ID"))
                        ImGui::SetClipboardText(idText.c_str());
                    if(ImGuiAux::StyledMenuItem("Copy Path"))
                        ImGui::SetClipboardText(meta.RelativePath.c_str());
                    ImGuiAux::EndStyledPopup();
                }

                ImGui::TableSetColumnIndex(COLUMN_TYPE);
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", SurgeReflect::EnumToString(meta.Type).data());

                ImGui::TableSetColumnIndex(COLUMN_STATUS);
                ImGui::PushFont(boldFont);
                if(HasFlag(meta.Flags, AssetFlags::MISSING))
                    ImGui::TextColored(ImVec4(0.9f, 0.2f, 0.2f, 1.0f), "MISSING");
                else if(HasFlag(meta.Flags, AssetFlags::MEMORY))
                    ImGui::TextColored(ImVec4(0.8f, 0.5f, 0.2f, 1.0f), "MEMORY");
                else if(HasFlag(meta.Flags, AssetFlags::LOADED))
                    ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "LOADED");
                else
                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "ON DISK");
                ImGui::PopFont();

                ImGui::TableSetColumnIndex(COLUMN_PATH);
                ImGui::TextUnformatted(meta.RelativePath.c_str());

                ImGui::TableSetColumnIndex(COLUMN_ACTIONS);
                ImGui::PushFont(boldFont);
                if(HasFlag(meta.Flags, AssetFlags::MEMORY))
                {
                    ImGui::BeginDisabled();
                    ImGuiAux::Button("BUILT-IN");
                    ImGui::EndDisabled();
                }
                else if(HasFlag(meta.Flags, AssetFlags::LOADED))
                {
                    const size_t refCount = mAssetManager->GetAssetRefCount(id);
                    if(refCount > 1)
                    {
                        ImGui::BeginDisabled();
                        ImGuiAux::Button(std::format("REFS: {}", refCount - 1).c_str());
                        ImGui::EndDisabled();
                        ImGuiAux::ItemTooltip("Referenced by the scene or another asset, can't be unloaded");
                    }
                    else
                    {
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.6f, 0.2f, 1.0f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.6f, 0.3f, 1.0f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.4f, 0.1f, 1.0f));
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
                        if(ImGuiAux::Button("UNLOAD"))
                            mToUnload.push_back(id);
                        ImGui::PopStyleColor(4);
                        ImGuiAux::ItemTooltip("Free this asset from GPU/CPU memory");
                    }
                }
                else
                {
                    const bool missing = HasFlag(meta.Flags, AssetFlags::MISSING);
                    ImGui::PushStyleColor(ImGuiCol_Button, missing ? ImVec4(0.8f, 0.2f, 0.2f, 1.0f) : ImVec4(0.8f, 0.6f, 0.2f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.3f, 0.3f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.1f, 0.1f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
                    if(ImGuiAux::Button("UNREGISTER"))
                        mToUnregister.push_back(id);
                    ImGui::PopStyleColor(4);
                    ImGuiAux::ItemTooltip("Removes the entry from the registry, the file stays on disk");
                }
                ImGui::PopFont();
                ImGui::PopID();
            }
        }
        ImGui::EndTable();
    }

} // namespace Surge
