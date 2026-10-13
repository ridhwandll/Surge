// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Graphics/UISystem/UILayout.hpp"

namespace Surge::UI
{
    /*
    * Clips its children to its rect and scrolls them by dragging (with inertia), the mouse wheel or from code
    * Children are laid out relative to the viewport, shifted by the scroll offset, the content size is the bounding box of the children
    * A press on a child (button...) that turns into a drag along a scrollable axis is taken over by the scroll view: the child gets no click
    */
    class ScrollView : public Widget
    {
    public:
        ScrollView();

        virtual WidgetType GetType() const override { return WidgetType::SCROLL_VIEW; }
        virtual void GenerateDrawCommands(DrawList& drawList) override;
        virtual Widget* HitTest(float x, float y) override;

        virtual void OnPointerDown(const glm::vec2& pos) override;
        virtual void OnPointerDrag(const glm::vec2& pos) override;
        virtual void OnPointerUp(const glm::vec2&) override { mIsPressed = false; mDragging = false; }
        virtual void CancelPress() override { mIsPressed = false; mDragging = false; }
        virtual bool WantsDrag() const override { return true; }
        virtual bool CanTakeOverDrag(const glm::vec2& delta) const override;
        virtual bool OnScroll(float wheelDelta) override;

        glm::vec2 GetScrollPosition() const;                  // Normalized 0..1 per axis, 0 = top/left
        void SetScrollPosition(const glm::vec2& normalized);  // Applied at the next layout (the content size is known then)
        glm::vec2 GetScrollOffset() const { return mScroll; } // UI units
        glm::vec2 GetContentSize() const { return mContentSize; }

    public:
        bool Horizontal = false;
        bool Vertical = true;
        float DecelerationRate = 0.135f; // Velocity kept after one second of coasting, 0 = no inertia
        float WheelSpeed = 120.0f;       // UI units per wheel notch
        bool ShowScrollbars = true;
        float ScrollbarWidth = 8.0f;     // UI units
        glm::vec4 ScrollbarColor = { 1.0f, 1.0f, 1.0f, 0.45f };

    protected:
        virtual void LayoutChildren() override;
        virtual void LayoutContent(const glm::vec2& origin); // Lays out the content at origin (pixels) and updates mContentSize
        virtual void OnScrollChanged(const glm::vec2& previous); // Default: moves the laid out children
        void SetScrollInternal(const glm::vec2& scroll);
        glm::vec2 GetMaxScroll() const;
        glm::vec2 ClampScroll(const glm::vec2& scroll) const { return glm::clamp(scroll, glm::vec2(0.0f), GetMaxScroll()); }
        glm::vec2 GetViewSize() const { return mGlobalSize / (mLayoutScale > 0.0f ? mLayoutScale : 1.0f); } // UI units

    private:
        void Tick(); // Inertia + scrollbar fade, once per frame

    protected:
        glm::vec2 mScroll = { 0.0f, 0.0f };      // UI units, so it survives resolution changes
        glm::vec2 mContentSize = { 0.0f, 0.0f }; // UI units

    private:
        glm::vec2 mVelocity = { 0.0f, 0.0f };
        glm::vec2 mPreviousScroll = { 0.0f, 0.0f };
        glm::vec2 mDragStartPos = { 0.0f, 0.0f };
        glm::vec2 mDragStartScroll = { 0.0f, 0.0f };
        glm::vec2 mPendingScroll = { 0.0f, 0.0f };
        bool mHasPendingScroll = false;
        bool mDragging = false;
        float mScrollbarAlpha = 0.0f;
        float mIdleTime = 0.0f;
    };

    // Lua/C++ hooks of a ListView (index is 0 based here, the Lua binding adds 1)
    class IListCallback : public RefCounted
    {
    public:
        virtual ~IListCallback() = default;
        virtual void BindItem(Widget* item, int index) = 0;
        virtual Ref<Widget> CreateItem() { return nullptr; }
    };

    /*
    * Scroll view for long lists: the first child in the layout is the item template, only about one screen of items exists
    * Items scrolling out are recycled for the ones scrolling in, OnBindItem(item, index) fills them. Fixed item size, optional grid (Columns)
    * Rows go down when Vertical scrolling is on, otherwise the items form one horizontal row
    */
    class ListView final : public ScrollView
    {
    public:
        virtual WidgetType GetType() const override { return WidgetType::LIST_VIEW; }
        virtual void Destroy() override;
        virtual void SetDesignPreview(bool preview) override { mDesignPreview = preview; }

        void SetItemTemplate(const WidgetDesc& desc) { mTemplate = desc; mHasTemplate = true; }
        void SetOnBindItem(Ref<IListCallback> callback) { mOnBindItem = callback; mNeedsRebind = true; }
        void SetOnCreateItem(Ref<IListCallback> callback) { mOnCreateItem = callback; }

        void SetItemCount(int count) { mItemCount = glm::max(count, 0); mNeedsRebind = true; }
        int GetItemCount() const { return mItemCount; }
        void Refresh() { mNeedsRebind = true; } // Rebinds the visible items (data changed)
        void ScrollToIndex(int index) { mPendingIndex = index; }

    public:
        glm::vec2 ItemSize = { 0.0f, 0.0f }; // UI units, 0 = the template's size (width 0 with one column = full width)
        glm::vec2 Spacing = { 0.0f, 10.0f };
        glm::vec4 Padding = { 0.0f, 0.0f, 0.0f, 0.0f }; // left, top, right, bottom
        int Columns = 1;

    protected:
        virtual void LayoutContent(const glm::vec2& origin) override;
        virtual void OnScrollChanged(const glm::vec2&) override;

    private:
        Ref<Widget> AcquireItem();
        glm::vec2 GetItemSize(float innerMainCross) const;

    private:
        WidgetDesc mTemplate;
        bool mHasTemplate = false;
        LayoutResourceCache mResourceCache;
        Ref<IListCallback> mOnBindItem;
        Ref<IListCallback> mOnCreateItem;

        Vector<int> mItemIndices; // Per child: bound data index, -1 = free (hidden)
        int mItemCount = 0;
        int mPendingIndex = -1;
        bool mNeedsRebind = false;
        bool mBinding = false;
        bool mDesignPreview = false;
    };

} // namespace Surge::UI
