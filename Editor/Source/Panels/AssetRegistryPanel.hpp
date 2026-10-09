// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Panels/IPanel.hpp"
#include "Surge/Asset/Asset.hpp"
#include "Surge/Core/Vector.hpp"

namespace Surge
{
    class AssetManager;

    // Every registered asset of the project: type, load state, references, unload/unregister
    class AssetRegistryPanel : public IPanel
    {
    public:
        AssetRegistryPanel() = default;
        virtual ~AssetRegistryPanel() override = default;
        static PanelCode GetStaticCode() { return PanelCode::AssetRegistry; }

        virtual void Init(void* panelInitArgs) override;
        virtual void OnEvent(Event&) override {}
        virtual void Render(bool* show) override;
        virtual void Shutdown() override {}

    private:
        void DrawToolbar();
        void DrawTable();
        void RebuildRows(); // Filter + sort, only when something changed

    private:
        PanelCode mCode;
        AssetManager* mAssetManager = nullptr;

        char mSearchBuffer[256] = "";
        AssetType mFilterType = AssetType::NONE; // NONE = every type

        Vector<AssetID> mRows;
        String mRowsSearch;
        AssetType mRowsFilter = AssetType::NONE;
        size_t mRowsRegistrySize = static_cast<size_t>(-1);
        int mSortColumn = 3; // Path
        bool mSortDescending = false;
        bool mSortDirty = true;

        Vector<AssetID> mToUnregister;
        Vector<AssetID> mToUnload;
    };

} // namespace Surge
