// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Panels/IPanel.hpp"
#include "Surge/ECS/Scene.hpp"
#include "Surge/Asset/Asset.hpp"
#include <unordered_map>

struct ImVec2;

namespace Surge
{
    class SceneHierarchyPanel : public IPanel
    {
    public:
        SceneHierarchyPanel() = default;
        ~SceneHierarchyPanel() = default;

        virtual void Init(void* panelInitArgs) override;
        virtual void OnEvent(Event& e) override;
        virtual void Render(bool* show) override;
        virtual void Shutdown() override;

        static PanelCode GetStaticCode() { return PanelCode::SceneHierarchy; }

        void SetSceneContext(Scene* scene)
        {
            SG_ASSERT_NOMSG(scene);
            mSelectedEntity = {};
            mSceneContext = scene;
        }
        Scene* GetSceneContext() { return mSceneContext; }

        void SetSelectedEntity(Entity& e)
        {
            mSelectedEntity = e;
            mRevealSelection = true; // Selected from elsewhere (Viewport, undo, paste...), open its parents and scroll to it
        }
        Entity& GetSelectedEntity() { return mSelectedEntity; }
        bool IsFocusedOrHovered() const { return mHierarchyFocused || mHierarchyHovered; }

        void StartRename(Entity entity);

    private:
        enum class CreateKind : uint8_t
        {
            NONE,
            EMPTY,
            CAMERA,
            SPRITE,
            MESH,
            ENVIRONMENT,
            DIRECTIONAL_LIGHT,
            POINT_LIGHT,
            TEXT,
            UI_CANVAS,
            AUDIO_SOURCE,
            AUDIO_LISTENER,
            PARTICLE_SYSTEM,
            FROM_ASSET // Dropped from the Content Browser
        };

        // Structural edits are queued while the tree is drawn and applied once it is done: they relink the very sibling chains being walked
        struct CreateRequest
        {
            CreateKind Kind = CreateKind::NONE;
            Entity Parent;
            const char* DefaultMesh = nullptr; // MESH: nullptr = empty MeshComponent
            AssetID Asset;                     // FROM_ASSET
        };

        struct MoveRequest
        {
            Entity Target;
            Entity Parent;
            Entity InsertAfter;
            bool KeepOrder = false; // Plain reparent (appended), no explicit sibling position
        };

        // Rows drawn this frame, top to bottom, for the arrow keys
        struct VisibleRow
        {
            entt::entity Handle;
            entt::entity Parent;
            bool HasChildren;
            bool Open;
        };

        void DrawToolbar();
        void DrawCreateMenu(Entity parent);
        void DrawEntityNode(Entity e, bool forceOpen);
        void DrawRowContents(Entity e, const ImVec2& rowMin, const ImVec2& rowMax, float labelX, bool isSelected, bool isRenaming);
        void DrawEntityContextMenu(Entity e);
        void AcceptDrop(Entity target, const ImVec2& rowMin, const ImVec2& rowMax, float labelX, bool targetOpen); // Null target: root level
        void DrawRenameField(Entity& e, const ImVec2& position);
        void HandleKeyboardNavigation();
        void ApplyPendingEdits();

        void Select(Entity e, bool reveal);
        void RequestOpen(entt::entity entity, bool open) { mOpenRequests.push_back({ entity, open }); }
        void UpdateSearch();
        bool IsSearching() const { return !mSearchName.empty() || !mSearchTypes.empty(); }
        bool MatchesTypeFilters(Entity e) const;

    private:
        bool mHierarchyHovered = false;
        bool mHierarchyFocused = false;
        bool mRevealSelection = false;

        // Inline rename
        Entity mRenamingEntity;
        String mRenameBuffer;
        bool mFocusRename = false;
        int mRenameGraceFrames = 0; // Frames the field may take to become active before the rename is dropped

        // Search: "name t:mesh t:light", name is a case insensitive substring, every t: token is a component filter
        String mSearchBuffer;
        String mLastSearchBuffer;
        bool mSearchChanged = false; // Results open fully once per query, collapsing them afterwards sticks
        String mSearchName; // Lower case
        Vector<String> mSearchTypes;
        bool mFocusSearch = false;
        std::unordered_map<entt::entity, bool> mSearchVisible; // Matches (true) and the ancestors that lead to them (false)

        Vector<entt::entity> mRoots;
        Vector<VisibleRow> mVisibleRows;
        Vector<std::pair<entt::entity, bool>> mOpenRequests; // Applied with SetNextItemOpen() the next time the row is drawn
        int mSetAllOpen = -1;                                // -1 none, 0 collapse all, 1 expand all
        entt::entity mExpandSubtree = entt::null;            // Opens this entity and all of its descendants
        entt::entity mNextExpandSubtree = entt::null;        // Requested while the tree is drawn, applied next frame
        entt::entity mPressedEntity = entt::null;            // Row pressed with the left button, selected on release unless it got dragged
        bool mOpenCreatePopup = false;
        bool mOpenSpacePopup = false;

        CreateRequest mPendingCreate;
        MoveRequest mPendingMove;

        PanelCode mCode;
        Scene* mSceneContext;
        Entity mSelectedEntity; // TODO: Make It a vector when we allow multiple selection
    };

} // namespace Surge
