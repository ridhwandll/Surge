// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Panels/IPanel.hpp"
#include "Surge/Graphics/HighLevel/Material.hpp"
#include "Surge/Graphics/HighLevel/Mesh.hpp"
#include "Surge/Graphics/Renderer/RenderView.hpp"
#include <glm/glm.hpp>
#include <imgui.h>
#include <array>
#include <filesystem>

namespace Surge
{
    /*
    * Material Editor: edits Material assets (.smat), glTF materials living inside a mesh are read-only
    *   - Live preview in its own isolated Renderer view: sphere/cube/plane/torus under a studio key + rim light
    *     (orbit: left drag, rotate the light: right drag, zoom: wheel, F resets), rendered only while it is on screen
    *   - Surface / Metallic & Roughness / Emission sections, each map next to the value it multiplies. Texture slots take
    *     Content Browser drops or the searchable picker, assigning a map turns its Use flag on and clearing it turns it off
    *   - Edits apply to the loaded asset right away (every mesh using it updates), Save (Ctrl+S) writes the .smat + the cooked file,
    *     Revert restores the last saved state
    *   - Undo/Redo (Ctrl+Z / Ctrl+Y) with snapshots of the property block + texture slots, a drag or a color pick is one step
    *   - "Make Editable Copy" turns a glTF material into a .smat next to its mesh and overrides the slot it was opened from
    */
    class MaterialEditorPanel : public IPanel
    {
    public:
        MaterialEditorPanel() = default;
        virtual ~MaterialEditorPanel() override = default;

        virtual void Init([[maybe_unused]] void* panelInitArgs) override;
        virtual void OnEvent([[maybe_unused]] Event& e) override {};
        virtual void Render(bool* show) override;
        virtual void Shutdown() override;

        // ownerMesh + slot: where a mesh slot was opened from (Inspector), the target of "Make Editable Copy". Asks before discarding unsaved changes
        void OpenMaterial(const Ref<Material>& material, const Ref<Mesh>& ownerMesh = nullptr, Uint slot = 0);
        void CloseMaterial(); // Asks before discarding unsaved changes
        bool OwnsShortcuts() const { return mOwnsShortcuts; }

        static PanelCode GetStaticCode() { return PanelCode::MaterialEditor; }

    public:
        static constexpr Uint TEXTURE_SLOT_COUNT = 4; // Albedo, Normal, RoughnessMetallic, Emission

    private:
        // Everything a user edits: the reflected property block + one texture asset per slot (raw AssetID, 0 = none: the white texture)
        struct Snapshot
        {
            Vector<Byte> Properties;
            std::array<uint64_t, TEXTURE_SLOT_COUNT> Textures = {};

            bool operator==(const Snapshot& other) const { return Properties == other.Properties && Textures == other.Textures; }
        };

        enum class PreviewShape { SPHERE = 0, CUBE, PLANE, TORUS, COUNT };

        struct PickerEntry
        {
            AssetID ID;
            String Name;
            String SearchKey; // Lower case relative path
            String Folder;    // Relative to Assets
            std::filesystem::path AbsolutePath;
            std::filesystem::file_time_type WriteTime;
        };

        // Document
        bool IsReadOnly() const { return mMaterial && !mMaterial->GetID(); }
        Snapshot TakeSnapshot() const;
        void RestoreSnapshot(const Snapshot& snapshot);
        void ResetDocument(); // Undo history + saved state follow the current material
        void Save();
        void Revert();
        void MarkChanged();
        void CommitUndoIfIdle();
        void Undo();
        void Redo();
        void HandleShortcuts();
        void MakeEditableCopy();

        // UI
        void DrawEmptyState();
        void DrawToolbar();
        void DrawReadOnlyBanner();
        void DrawProperties();
        bool BeginSection(const char* name);
        void EndSection();
        void DrawTextureSlot(const char* label, Uint slot);
        void SetSlotTexture(Uint slot, const Ref<Texture2D>& texture);
        void OpenTexturePicker(Uint slot);
        void DrawTexturePicker();

        // Preview
        void DrawPreview();
        void DrawPreviewToolbar(const ImVec2& min);
        void HandlePreviewInput(bool hovered);
        void UpdatePreview();
        void EnsurePreviewMesh();
        void ResetPreviewCamera();
        glm::mat4 GetPreviewView() const;
        glm::mat4 GetPreviewProjection(float aspect) const;

    private:
        PanelCode mCode = PanelCode::MaterialEditor;
        Ref<Material> mMaterial;
        Ref<Mesh> mOwnerMesh;
        Uint mOwnerSlot = 0;
        bool mUnsaved = false;
        bool mDocumentChanged = false; // Edited since the last undo step was committed
        bool mOwnsShortcuts = false;
        bool mFocusWindow = false;

        // Undo
        static constexpr size_t MAX_UNDO_STEPS = 64;
        Vector<Snapshot> mUndoStack;
        Vector<Snapshot> mRedoStack;
        Snapshot mCommitted;
        Snapshot mSaved;

        // Texture picker
        Uint mPickerSlot = 0;
        bool mOpenPicker = false;
        String mPickerFilter;
        Vector<PickerEntry> mPickerEntries;

        // Preview
        RenderViewID mView = INVALID_RENDER_VIEW;
        Ref<Mesh> mPreviewMesh;
        Ref<Material> mPreviewPlaceholder; // The preview mesh's own slot, always overridden by the edited material
        PreviewShape mPreviewShape = PreviewShape::SPHERE;
        PreviewShape mBuiltShape = PreviewShape::COUNT;
        bool mPreviewOpen = true;
        bool mPreviewVisible = false; // Drawn this frame (the view only renders while it is on screen)
        bool mAutoRotate = false;
        bool mDragging = false;       // The preview's input item is active: a camera/light drag owns the mouse
        ImVec2 mDragAnchor = { 0.0f, 0.0f };
        ImVec2 mPreviewSize = { 0.0f, 0.0f };
        float mPreviewHeight = 240.0f;
        float mMeshYaw = 0.0f;
        float mYaw = 0.6f;
        float mPitch = 0.3f;
        float mDistance = 2.0f;
        float mLightYaw = -0.8f;
        float mLightPitch = 0.75f;
        float mAmbient = 1.0f; // Scales the preview's studio ambient
    };
} // namespace Surge
