// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Graphics/UISystem/UIScrollView.hpp"

namespace Surge::UI
{
    /*
    * Container shown above every canvas while open, with a dimmed backdrop that blocks the input below it
    * Its children are its content, laid out inside its own rect. Show()/Hide() from lua, optional close on backdrop tap and on Back
    * In the UI Editor it is drawn in place so its content can be edited
    */
    class Modal final : public Widget
    {
    public:
        Modal();

        virtual WidgetType GetType() const override { return WidgetType::MODAL; }
        virtual void GenerateDrawCommands(DrawList& drawList) override;
        virtual Widget* HitTest(float x, float y) override;
        virtual void Destroy() override;
        virtual void SetDesignPreview(bool preview) override { mDesignPreview = preview; }

        virtual void OnPointerDown(const glm::vec2& pos) override;
        virtual void OnPointerUp(const glm::vec2& pos) override;

        virtual void GenerateOverlayDrawCommands(DrawList& drawList) override;
        virtual Widget* OverlayHitTest(float x, float y) override;
        virtual bool IsOverlayModal() const override { return true; }
        virtual void OnOverlayDismiss() override { if(CloseOnBack) Hide(); }

        void Show();
        void Hide();
        bool IsOpen() const { return mOpen; }
        void SetOnClose(Ref<IEventCallback> callback) { mOnClose = callback; }

    public:
        glm::vec4 BackdropColor = { 0.0f, 0.0f, 0.0f, 0.6f };
        bool CloseOnBackdrop = true;
        bool CloseOnBack = true;
        ImageStyle Panel;
        glm::vec4 PanelColor = { 0.14f, 0.15f, 0.18f, 1.0f };

    private:
        Ref<IEventCallback> mOnClose;
        bool mOpen = false;
        bool mDesignPreview = false;
        bool mPressedOutside = false;
    };

    // Picks one of Options. The open list is an overlay (drawn above everything, scrolls past MaxVisibleItems), a tap outside closes it
    class Dropdown final : public Widget
    {
    public:
        Dropdown(Ref<Font> font);

        virtual WidgetType GetType() const override { return WidgetType::DROPDOWN; }
        virtual void GenerateDrawCommands(DrawList& drawList) override;
        virtual void OnPointerUp(const glm::vec2& pos) override;
        virtual void Destroy() override;

        virtual void GenerateOverlayDrawCommands(DrawList& drawList) override;
        virtual Widget* OverlayHitTest(float x, float y) override;
        virtual void OnOverlayDismiss() override { Close(); }

        void SetOptions(const Vector<String>& options);
        const Vector<String>& GetOptions() const { return mOptions; }
        void SetSelectedIndex(int index, bool notify = true); // -1 = nothing selected
        int GetSelectedIndex() const { return mSelectedIndex; }
        String GetSelectedText() const { return mSelectedIndex >= 0 && mSelectedIndex < static_cast<int>(mOptions.size()) ? mOptions[mSelectedIndex] : String(); }
        void SetFont(Ref<Font> font);
        void Open();
        void Close();
        bool IsOpen() const { return mOpen; }

    public:
        int MaxVisibleItems = 5;
        float FontSize = 30.0f;
        float TextPadding = 20.0f; // UI units
        String Placeholder = "Select...";
        ImageStyle Background;
        ImageStyle PopupStyle;
        glm::vec4 BackgroundColor = { 0.2f, 0.21f, 0.25f, 1.0f };
        glm::vec4 PopupColor = { 0.16f, 0.17f, 0.2f, 1.0f };
        glm::vec4 TextColor = { 1.0f, 1.0f, 1.0f, 1.0f };
        glm::vec4 ItemHoverColor = { 1.0f, 1.0f, 1.0f, 0.08f };
        glm::vec4 SelectedItemColor = { 0.25f, 0.6f, 1.0f, 0.35f };

    protected:
        virtual void LayoutChildren() override;

    private:
        void RebuildPopup();
        void UpdateLabel();

    private:
        Vector<String> mOptions;
        int mSelectedIndex = -1;
        bool mOpen = false;
        bool mPopupDirty = true;
        Ref<Font> mFont;
        Ref<Text> mLabel;
        Ref<Widget> mPopupRoot;       // Full screen root of the open list, never part of a canvas
        Ref<ScrollView> mPopupScroll;
        Ref<LayoutGroup> mPopupItems;
    };

} // namespace Surge::UI
