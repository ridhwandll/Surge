// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Panels/IPanel.hpp"
#include "Asset/ThumbnailCache.hpp"
#include "Surge/Asset/Asset.hpp"
#include "Surge/Core/Path.hpp"
#include "Surge/Graphics/RHI/RHIHandle.hpp"
#include <imgui.h>
#include <unordered_map>

#define CONTENT_BROWSER_PAYLOAD "CONTENT_BROWSER_ASSET"       // AssetID of one registered asset, what the Inspector/Viewport/... accept
#define CONTENT_BROWSER_ITEMS_PAYLOAD "CONTENT_BROWSER_ITEMS" // '\n' separated absolute paths: several items, folders, unimported files

namespace Surge
{
    class AssetManager;
    struct ContentBrowserSettings;

    /*
     * Content Browser
     * - Folder tree with favorites | breadcrumbs, history, search (current folder or the whole project), type filter, sorting
     * - Grid (thumbnails, Ctrl+Wheel zoom) and list (sortable columns) views, multi-selection with Ctrl/Shift/box select and keyboard navigation
     * - Create, rename (F2), duplicate, cut/copy/paste, move by drag & drop (onto folders, the tree or the breadcrumbs), delete to the Recycle Bin
     * - Import from the OS (drop files on the panel or use Import), unimported files are shown and can be imported in place
     * Every file operation goes through AssetFileOps, so registered assets keep their AssetID when they are moved or renamed
     * - Content Drawer (Ctrl+Space, like Unreal): the same browser slides up from the bottom over the editor, also while its tab is closed
     */
    class ContentBrowserPanel : public IPanel
    {
    public:
        enum class SortField
        {
            NAME = 0,
            TYPE,
            SIZE,
            MODIFIED
        };

        ContentBrowserPanel() = default;
        virtual ~ContentBrowserPanel() override = default;
        static PanelCode GetStaticCode() { return PanelCode::ContentBrowser; }

        virtual void Init(void* panelInitArgs) override;
        virtual void OnEvent(Event& e) override;
        virtual void Render(bool* show) override;
        virtual void Shutdown() override;

        void OnAssetManagerInit();      // Another project: back to the Assets root
        void OnProjectSettingsLoaded(); // Reopens the folder that was open last time
        void SetSelectedAsset(AssetID asset); // Shows the asset's folder and selects it
        void RequestRefresh() { mNeedsRefresh = true; }
        void ReleaseGPUResources(); // Icons + thumbnails, before the Editor's icon sampler goes away
        ThumbnailCache& GetThumbnails() { return mThumbnails; } // Shared with the asset pickers, uploads only while the browser is shown

        // Closes again on Ctrl+Space, Esc, a click outside or a drag leaving it (dropping into a viewport/the Inspector)
        void ToggleDrawer() { SetDrawerOpen(!mDrawerOpen); }
        void SetDrawerOpen(bool open);
        bool IsDrawerOpen() const { return mDrawerOpen; }

    private:
        struct Item
        {
            Path AbsolutePath;
            String Name;        // File name with extension
            String DisplayName; // Registered files drop the extension, the type badge tells it
            String SortKey;     // Lower case name
            String Location;    // Parent folder relative to Assets (search results)
            String SizeText;
            String ModifiedText;
            std::filesystem::file_time_type WriteTime;
            uintmax_t Size = 0;
            ImGuiID SelectionID = 0;
            AssetID ID = AssetID::INVALID;
            AssetType Type = AssetType::NONE; // Registered type, or the one the extension would import as
            bool IsDirectory = false;
            bool IsEmptyDirectory = false;
            bool IsRegistered = false;
            bool IsMissing = false;
            bool IsImage = false; // Gets a thumbnail
        };

        struct FolderNode
        {
            Path AbsolutePath;
            String Name;
            Vector<FolderNode> Children;
        };

        // Data
        void Refresh();
        void RebuildFolderTree();
        void ScanFolder(FolderNode& node);
        void RebuildItems();
        Item MakeItem(const std::filesystem::directory_entry& entry, const std::unordered_map<String, AssetID>& assetsByPath) const;
        void SortAndFilter();
        bool PassesFilter(const Item& item) const;
        void CheckForExternalChanges();
        void ApplyPendingSelection();

        // Navigation
        void Navigate(const Path& folder, bool recordHistory = true);
        void NavigateBack();
        void NavigateForward();
        void NavigateUp();

        // UI
        void DrawBody(); // Everything inside the window, shared by the tab and the drawer (only one of them draws it per frame)
        void RenderDrawer();
        void DrawDrawerHeader(float areaHeight);
        void DrawToolbar();
        void DrawBreadcrumbs(float width);
        void DrawFilterPopup();
        void DrawSortPopup();
        void DrawFolderTree();
        void DrawFolderNode(const FolderNode& node, bool isRoot);
        void DrawFolderContextMenu(const Path& folder, bool isRoot);
        void DrawSplitter(float height);
        void DrawContent();
        void DrawGrid();
        void DrawList();
        void DrawTile(Item& item, int index, const ImVec2& pos, const ImVec2& size);
        void DrawItemIcon(ImDrawList* drawList, const Item& item, const ImVec2& min, const ImVec2& max, float alpha);
        void DrawRenameField(float width);
        void HandleItemInteraction(Item& item, bool selected);
        void DrawItemContextMenu();
        void DrawBackgroundContextMenu();
        void DrawCreateMenuItems(const Path& folder);
        void DrawEmptyHint();
        void DrawStatusBar();
        void UpdateThumbnailTask(size_t pending); // Editor status bar progress
        void HandleShortcuts();
        void HandleOsDrop();
        bool AcceptDrop(const Path& folder); // Drop target on the last item: moves dragged content into the folder

        // Operations
        Vector<const Item*> GetSelection() const;
        Vector<Path> GetSelectedPaths() const;
        void OpenItem(const Item& item);
        void BeginRename(const Path& path);
        void CommitRename();
        void DeletePaths(const Vector<Path>& paths);
        void Duplicate(const Vector<Path>& paths);
        void MoveInto(const Vector<Path>& paths, const Path& folder);
        void PasteInto(const Path& folder);
        void ImportExternal(const Vector<String>& files, const Path& folder);
        void ImportUnregistered(const Vector<Path>& paths);
        void Reimport(const Vector<const Item*>& items);
        void CreateFolderIn(const Path& folder);
        void CreateAssetIn(AssetType type, const Path& folder);
        void SelectAfterRefresh(Vector<Path> paths, bool rename = false);
        void OnPathMoved(const Path& from, const Path& to); // Current folder, history, clipboard and favorites follow a rename/move
        bool IsProtected(const Vector<Path>& paths, String& outReason) const; // The open scene can't be deleted
        void Notify(String message, bool isError = false);
        ContentBrowserSettings& Settings() const;

    private:
        PanelCode mCode;
        AssetManager* mAssetManager = nullptr;
        ThumbnailCache mThumbnails;
        size_t mThumbnailTaskCount = 0;

        ImageHandle mFolderIcon;
        ImageHandle mEmptyFolderIcon;
        ImageHandle mFileIcon;
        ImTextureID mFolderIconID = 0;
        ImTextureID mEmptyFolderIconID = 0;
        ImTextureID mFileIconID = 0;

        Path mBaseDirectory;
        Path mCurrentDirectory;
        Vector<Path> mBackHistory;
        Vector<Path> mForwardHistory;

        FolderNode mFolderTree;
        Vector<Item> mItems;
        Vector<int> mVisible; // Indices into mItems, filtered + sorted, what the views draw
        ImGuiSelectionBasicStorage mSelection;
        bool mNeedsRefresh = true;
        bool mNeedsSort = true;
        bool mRevealInTree = true;

        // External changes (Explorer, other panels)
        float mChangeCheckTimer = 0.0f;
        std::filesystem::file_time_type mCurrentDirectoryWriteTime;
        size_t mRegistrySize = 0;

        // Search
        char mSearchBuffer[256] = "";
        String mAppliedSearch;
        bool mFocusSearch = false;

        // Rename
        Path mRenamingPath;
        String mRenameBuffer;
        String mRenameExtension; // Files rename their stem, the extension (= asset type) stays
        bool mFocusRename = false;
        int mRenameGraceFrames = 0;

        // Applied once the next refresh rebuilt the items
        Vector<Path> mSelectAfterRefresh;
        bool mRenameAfterRefresh = false;
        Path mScrollToPath;

        // Clipboard (Ctrl+X/C/V)
        Vector<Path> mClipboard;
        bool mClipboardCut = false;

        bool mOpenItemContext = false;

        // Files dropped from the OS, handled on the next Render with the panel's rect
        Vector<String> mDroppedFiles;
        ImVec2 mDropPoint = { 0.0f, 0.0f };

        // Status bar message
        String mStatusMessage;
        bool mStatusIsError = false;
        float mStatusTimer = 0.0f;

        // Content Drawer
        bool mDrawerOpen = false;
        float mDrawerAnimation = 0.0f; // 0 = closed .. 1 = open, drawn until it slid out
        bool mFocusDrawer = false;
        bool mDrawerWasFocused = false;
        bool mDrawerDragTracked = false; // A drag in progress was seen, mDragFromDrawer tells where it started
        bool mDragFromDrawer = false;
        bool mFocusTab = false;          // Dock in Layout
    };

} // namespace Surge
