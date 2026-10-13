// Copyright (c) - SurgeTechnologies - All rights reserved
#include "UIScrollView.hpp"
#include <cmath>

namespace Surge::UI
{
    // -----------------------------------------------------------------------------------------------
    // ScrollView
    // -----------------------------------------------------------------------------------------------

    ScrollView::ScrollView()
    {
        mIsInteractable = true; // Drags on empty space scroll too
        mSize = { 600.0f, 800.0f };
    }

    glm::vec2 ScrollView::GetMaxScroll() const
    {
        return glm::max(mContentSize - GetViewSize(), glm::vec2(0.0f));
    }

    glm::vec2 ScrollView::GetScrollPosition() const
    {
        const glm::vec2 maxScroll = GetMaxScroll();
        return { maxScroll.x > 0.0f ? mScroll.x / maxScroll.x : 0.0f, maxScroll.y > 0.0f ? mScroll.y / maxScroll.y : 0.0f };
    }

    void ScrollView::SetScrollPosition(const glm::vec2& normalized)
    {
        mPendingScroll = glm::clamp(normalized, glm::vec2(0.0f), glm::vec2(1.0f));
        mHasPendingScroll = true;
        mVelocity = { 0.0f, 0.0f };
    }

    void ScrollView::LayoutContent(const glm::vec2& origin)
    {
        // Children relative to the viewport rect, moved by the scroll offset. Content = bounding box of the visible children (from the origin)
        glm::vec2 extent = { 0.0f, 0.0f };
        for(auto& child : mChildren)
        {
            child->UpdateLayout(origin, mGlobalSize, mLayoutScale);
            if(!child->IsVisible())
                continue;

            auto [pos, size] = child->GetGlobalBounds();
            extent = glm::max(extent, pos + size - origin);
        }
        mContentSize = glm::max(extent / mLayoutScale, GetViewSize());
    }

    void ScrollView::LayoutChildren()
    {
        LayoutContent(mGlobalPos - mScroll * mLayoutScale);

        // The content size is only known now: apply a pending SetScrollPosition and keep the offset inside the (maybe smaller) content
        glm::vec2 target = mScroll;
        if(mHasPendingScroll)
        {
            target = mPendingScroll * GetMaxScroll();
            mHasPendingScroll = false;
        }
        SetScrollInternal(ClampScroll(target));
    }

    void ScrollView::OnScrollChanged(const glm::vec2& previous)
    {
        const glm::vec2 delta = (previous - mScroll) * mLayoutScale;
        for(auto& child : mChildren)
            child->TranslateLayout(delta);
    }

    void ScrollView::SetScrollInternal(const glm::vec2& scroll)
    {
        if(scroll == mScroll)
            return;

        const glm::vec2 previous = mScroll;
        mScroll = scroll;
        OnScrollChanged(previous);
    }

    void ScrollView::Tick()
    {
        const float dt = GetFrameInfo().DeltaTime;
        if(dt <= 0.0f)
            return;

        if(mDragging)
        {
            // Smoothed release velocity, like Unity's ScrollRect
            const glm::vec2 frameVelocity = (mScroll - mPreviousScroll) / dt;
            mVelocity = glm::mix(mVelocity, frameVelocity, glm::min(dt * 10.0f, 1.0f));
        }
        else if(glm::length(mVelocity) > 1.0f && DecelerationRate > 0.0f)
        {
            // Exponential decay: velocity * rate^dt keeps DecelerationRate of the speed after one second, frame rate independent
            const glm::vec2 unclamped = mScroll + mVelocity * dt;
            const glm::vec2 clamped = ClampScroll(unclamped);
            mVelocity *= std::pow(glm::clamp(DecelerationRate, 0.0f, 0.999f), dt);
            if(clamped.x != unclamped.x) mVelocity.x = 0.0f; // Hit an end
            if(clamped.y != unclamped.y) mVelocity.y = 0.0f;
            SetScrollInternal(clamped);
        }
        else
            mVelocity = { 0.0f, 0.0f };

        mPreviousScroll = mScroll;

        // Scrollbars show while moving and fade out shortly after
        if(mDragging || glm::length(mVelocity) > 1.0f)
        {
            mScrollbarAlpha = 1.0f;
            mIdleTime = 0.0f;
        }
        else
        {
            mIdleTime += dt;
            if(mIdleTime > 0.6f)
                mScrollbarAlpha = glm::max(mScrollbarAlpha - dt * 3.0f, 0.0f);
        }
    }

    void ScrollView::GenerateDrawCommands(DrawList& drawList)
    {
        Tick();

        const uint16_t previousClip = drawList.PushClip(mGlobalPos, mGlobalPos + mGlobalSize);
        GenerateChildDrawCommands(drawList);

        if(ShowScrollbars && mScrollbarAlpha > 0.0f)
        {
            const glm::vec2 view = GetViewSize();
            const glm::vec2 maxScroll = GetMaxScroll();
            const float width = ScrollbarWidth * mLayoutScale;
            const float margin = 3.0f * mLayoutScale;
            glm::vec4 color = ScrollbarColor * mColor;
            color.a *= mScrollbarAlpha;

            // Thumb length = visible fraction of the content, position = scroll fraction of the remaining track
            if(Vertical && maxScroll.y > 0.0f)
            {
                const float track = mGlobalSize.y - margin * 2.0f;
                const float length = glm::max(track * view.y / mContentSize.y, width * 3.0f);
                const float y = mGlobalPos.y + margin + (track - length) * (mScroll.y / maxScroll.y);
                const float x = mGlobalPos.x + mGlobalSize.x - width - margin;
                DrawRect(drawList, { x, y }, { x + width, y + length }, color, width * 0.5f);
            }
            if(Horizontal && maxScroll.x > 0.0f)
            {
                const float track = mGlobalSize.x - margin * 2.0f;
                const float length = glm::max(track * view.x / mContentSize.x, width * 3.0f);
                const float x = mGlobalPos.x + margin + (track - length) * (mScroll.x / maxScroll.x);
                const float y = mGlobalPos.y + mGlobalSize.y - width - margin;
                DrawRect(drawList, { x, y }, { x + length, y + width }, color, width * 0.5f);
            }
        }

        drawList.PopClip(previousClip);
    }

    Widget* ScrollView::HitTest(float x, float y)
    {
        // Clipped: content outside the viewport can't be tapped
        if(!mVisible || !ContainsPoint(x, y))
            return nullptr;

        if(Widget* hit = HitTestChildren(x, y))
            return hit;
        return mIsInteractable ? this : nullptr;
    }

    void ScrollView::OnPointerDown(const glm::vec2& pos)
    {
        mIsPressed = true;
        mDragging = true;
        mDragStartPos = pos;
        mDragStartScroll = mScroll;
        mPreviousScroll = mScroll;
        mVelocity = { 0.0f, 0.0f };
    }

    void ScrollView::OnPointerDrag(const glm::vec2& pos)
    {
        if(!mDragging)
            return;

        // Content follows the finger 1:1 from where it first touched
        const glm::vec2 delta = (pos - mDragStartPos) / mLayoutScale;
        glm::vec2 target = mScroll;
        if(Horizontal) target.x = mDragStartScroll.x - delta.x;
        if(Vertical)   target.y = mDragStartScroll.y - delta.y;
        SetScrollInternal(ClampScroll(target));
    }

    bool ScrollView::CanTakeOverDrag(const glm::vec2& delta) const
    {
        const glm::vec2 maxScroll = GetMaxScroll();
        const bool vertical = Vertical && maxScroll.y > 0.0f && std::abs(delta.y) >= std::abs(delta.x);
        const bool horizontal = Horizontal && maxScroll.x > 0.0f && std::abs(delta.x) >= std::abs(delta.y);
        return vertical || horizontal;
    }

    bool ScrollView::OnScroll(float wheelDelta)
    {
        const glm::vec2 maxScroll = GetMaxScroll();
        glm::vec2 target = mScroll;
        if(Vertical && maxScroll.y > 0.0f)
            target.y -= wheelDelta * WheelSpeed;
        else if(Horizontal && maxScroll.x > 0.0f)
            target.x -= wheelDelta * WheelSpeed;
        else
            return false;

        mVelocity = { 0.0f, 0.0f };
        mScrollbarAlpha = 1.0f;
        mIdleTime = 0.0f;
        SetScrollInternal(ClampScroll(target));
        return true;
    }

    // -----------------------------------------------------------------------------------------------
    // ListView
    // -----------------------------------------------------------------------------------------------

    void ListView::Destroy()
    {
        mOnBindItem = nullptr;
        mOnCreateItem = nullptr;
        mItemIndices.clear();
        mResourceCache.Clear();
        ScrollView::Destroy();
    }

    Ref<Widget> ListView::AcquireItem()
    {
        // Free (scrolled out) item first, a new one only when the visible range grew
        for(size_t i = 0; i < mChildren.size(); i++)
        {
            if(mItemIndices[i] < 0)
                return mChildren[i];
        }

        Ref<Widget> item;
        if(mOnCreateItem)
            item = mOnCreateItem->CreateItem();
        else if(mHasTemplate)
            item = Instantiate(mTemplate, mResourceCache);

        if(item)
        {
            AddChild(item);
            mItemIndices.push_back(-1);
        }
        return item;
    }

    glm::vec2 ListView::GetItemSize(float innerCross) const
    {
        const glm::vec2 templateSize = mHasTemplate ? mTemplate.Size : (mChildren.empty() ? glm::vec2(100.0f) : mChildren[0]->GetSize());
        glm::vec2 size = { ItemSize.x > 0.0f ? ItemSize.x : templateSize.x, ItemSize.y > 0.0f ? ItemSize.y : templateSize.y };

        // Zero size across the scroll direction: share the viewport between the columns (full width rows)
        const int columns = glm::max(Columns, 1);
        if(Vertical && ItemSize.x <= 0.0f && (templateSize.x <= 0.0f || columns == 1))
            size.x = (innerCross - Spacing.x * (columns - 1)) / static_cast<float>(columns);
        else if(!Vertical && ItemSize.y <= 0.0f && (templateSize.y <= 0.0f || columns == 1))
            size.y = (innerCross - Spacing.y * (columns - 1)) / static_cast<float>(columns);
        return glm::max(size, glm::vec2(1.0f));
    }

    void ListView::LayoutContent(const glm::vec2&)
    {
        mItemIndices.resize(mChildren.size(), -1);
        const float scale = mLayoutScale;
        const glm::vec2 view = GetViewSize();
        const bool vertical = Vertical || !Horizontal;
        const int columns = glm::max(Columns, 1);

        // Main = scroll axis, cross = the other one. In design preview the existing children are placeholder items
        const float innerCross = vertical ? view.x - Padding.x - Padding.z : view.y - Padding.y - Padding.w;
        const glm::vec2 itemSize = GetItemSize(innerCross);
        const float itemMain = vertical ? itemSize.y : itemSize.x;
        const float itemCross = vertical ? itemSize.x : itemSize.y;
        const float spacingMain = vertical ? Spacing.y : Spacing.x;
        const float spacingCross = vertical ? Spacing.x : Spacing.y;
        const float paddingMainStart = vertical ? Padding.y : Padding.x;
        const float paddingMainEnd = vertical ? Padding.w : Padding.z;
        const float paddingCrossStart = vertical ? Padding.x : Padding.y;

        const int count = mDesignPreview ? static_cast<int>(mChildren.size()) : mItemCount;
        const int lines = (count + columns - 1) / columns;
        const float stride = glm::max(itemMain + spacingMain, 1.0f);
        const float contentMain = paddingMainStart + paddingMainEnd + (lines > 0 ? lines * itemMain + (lines - 1) * spacingMain : 0.0f);
        mContentSize = vertical ? glm::vec2(view.x, glm::max(contentMain, view.y)) : glm::vec2(glm::max(contentMain, view.x), view.y);

        if(mPendingIndex >= 0 && !mDesignPreview)
        {
            const float lineStart = paddingMainStart + static_cast<float>(glm::min(mPendingIndex, glm::max(count - 1, 0)) / columns) * stride;
            glm::vec2 target = mScroll;
            (vertical ? target.y : target.x) = lineStart;
            mPendingIndex = -1;
            mScroll = ClampScroll(target);
        }
        const glm::vec2 itemOrigin = mGlobalPos - mScroll * scale; // == origin unless ScrollToIndex moved the scroll above

        // Visible lines: (scroll - padding) / stride, one extra line keeps partially visible rows bound
        const float scrollMain = vertical ? mScroll.y : mScroll.x;
        const float viewMain = vertical ? view.y : view.x;
        const int firstLine = glm::max(static_cast<int>(std::floor((scrollMain - paddingMainStart) / stride)), 0);
        const int lastLine = glm::min(static_cast<int>(std::floor((scrollMain + viewMain - paddingMainStart) / stride)), lines - 1);
        const int first = firstLine * columns;
        const int last = glm::min((lastLine + 1) * columns, count) - 1;

        if(mDesignPreview)
        {
            for(size_t i = 0; i < mChildren.size(); i++)
                mItemIndices[i] = static_cast<int>(i);
        }
        else
        {
            // Recycle items that left the range
            for(size_t i = 0; i < mChildren.size(); i++)
            {
                if(mItemIndices[i] >= 0 && (mItemIndices[i] < first || mItemIndices[i] > last || mItemIndices[i] >= count))
                {
                    mItemIndices[i] = -1;
                    mChildren[i]->SetVisible(false);
                }
            }

            // Bind the ones that entered it (and all of them after Refresh / ItemCount changes)
            const bool rebindAll = mNeedsRebind;
            mNeedsRebind = false; // A callback may set it again (ItemCount from Lua), handled next layout
            mBinding = true;
            for(int index = first; index <= last; index++)
            {
                int slot = -1;
                for(size_t i = 0; i < mItemIndices.size(); i++)
                {
                    if(mItemIndices[i] == index)
                    {
                        slot = static_cast<int>(i);
                        break;
                    }
                }

                const bool isNew = slot < 0;
                if(isNew)
                {
                    Ref<Widget> item = AcquireItem();
                    if(!item)
                        break; // No template and no OnCreateItem
                    for(size_t i = 0; i < mChildren.size(); i++)
                    {
                        if(mChildren[i].get() == item.get())
                            slot = static_cast<int>(i);
                    }
                    mItemIndices[slot] = index;
                    item->SetVisible(true);
                }

                if((isNew || rebindAll) && mOnBindItem)
                    mOnBindItem->BindItem(mChildren[slot].get(), index);
            }
            mBinding = false;
        }

        // Place every bound item on its line/column
        for(size_t i = 0; i < mChildren.size(); i++)
        {
            const int index = mItemIndices[i];
            if(index < 0)
                continue;

            const int line = index / columns;
            const int column = index % columns;
            const float main = paddingMainStart + static_cast<float>(line) * stride;
            const float cross = paddingCrossStart + static_cast<float>(column) * (itemCross + spacingCross);
            const glm::vec2 offset = vertical ? glm::vec2(cross, main) : glm::vec2(main, cross);
            mChildren[i]->LayoutAt(itemOrigin + offset * scale, itemSize * scale, scale);
        }
    }

    void ListView::OnScrollChanged(const glm::vec2&)
    {
        // Other items become visible: rebind and place them, not just move them
        if(!mBinding)
            LayoutContent(mGlobalPos - mScroll * mLayoutScale);
    }
}
