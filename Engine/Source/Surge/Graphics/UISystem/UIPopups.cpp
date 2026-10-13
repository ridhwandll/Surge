// Copyright (c) - SurgeTechnologies - All rights reserved
#include "UIPopups.hpp"
#include "Surge/Core/Core.hpp"
#include "Surge/Graphics/Renderer/Renderer.hpp"

namespace Surge::UI
{
    static Manager& GetManager()
    {
        return Core::GetRenderer()->GetUIManager();
    }

    // -----------------------------------------------------------------------------------------------
    // Modal
    // -----------------------------------------------------------------------------------------------

    Modal::Modal()
    {
        mSize = { 700.0f, 500.0f };
        Panel.CornerRadius = 24.0f;
    }

    void Modal::Show()
    {
        if(mOpen || mDesignPreview)
            return;

        mOpen = true;
        GetManager().OpenOverlay(Ref<Widget>(this));
    }

    void Modal::Hide()
    {
        if(!mOpen)
            return;

        mOpen = false;
        mIsPressed = false;
        GetManager().CloseOverlay(this);
        if(mOnClose)
            mOnClose->Invoke();
    }

    void Modal::Destroy()
    {
        if(mOpen)
        {
            mOpen = false;
            GetManager().CloseOverlay(this); // No OnClose during teardown
        }
        mOnClose = nullptr;
        Widget::Destroy();
    }

    void Modal::GenerateDrawCommands(DrawList& drawList)
    {
        // At runtime the content is drawn by the overlay pass (above every canvas), the editor shows it in place
        if(!mDesignPreview)
            return;

        DrawImage(drawList, Panel, PanelColor * mColor, mGlobalPos, mGlobalSize, mLayoutScale);
        GenerateChildDrawCommands(drawList);
    }

    Widget* Modal::HitTest(float x, float y)
    {
        return mDesignPreview ? Widget::HitTest(x, y) : nullptr; // Only reachable through the overlay while open
    }

    void Modal::GenerateOverlayDrawCommands(DrawList& drawList)
    {
        DrawRect(drawList, { 0.0f, 0.0f }, GetFrameInfo().ScreenSize, BackdropColor);
        DrawImage(drawList, Panel, PanelColor * mColor, mGlobalPos, mGlobalSize, mLayoutScale);
        GenerateChildDrawCommands(drawList);
    }

    Widget* Modal::OverlayHitTest(float x, float y)
    {
        if(Widget* hit = HitTestChildren(x, y))
            return hit;

        return this; // Panel and backdrop both swallow the press, the backdrop may close on release
    }

    void Modal::OnPointerDown(const glm::vec2& pos)
    {
        mIsPressed = true;
        mPressedOutside = !ContainsPoint(pos.x, pos.y);
    }

    void Modal::OnPointerUp(const glm::vec2& pos)
    {
        // Pressed and released on the backdrop
        const bool close = mIsPressed && mPressedOutside && !ContainsPoint(pos.x, pos.y) && CloseOnBackdrop;
        mIsPressed = false;
        if(close)
            Hide();
    }

    // -----------------------------------------------------------------------------------------------
    // Dropdown
    // -----------------------------------------------------------------------------------------------

    class DropdownItemCallback final : public IEventCallback
    {
    public:
        DropdownItemCallback(Dropdown* owner, int index) : mOwner(owner), mIndex(index) {}
        void Invoke() override
        {
            Dropdown* owner = mOwner; // Close() may drop the popup that owns this callback
            const int index = mIndex;
            owner->Close();
            owner->SetSelectedIndex(index);
        }

    private:
        Dropdown* mOwner;
        int mIndex;
    };

    Dropdown::Dropdown(Ref<Font> font)
        : mFont(font)
    {
        mIsInteractable = true;
        mSize = { 400.0f, 80.0f };
        Background.CornerRadius = 12.0f;
        PopupStyle.CornerRadius = 12.0f;

        mLabel = Ref<Text>::Create(Placeholder, font);
        mLabel->SetTextAlignment(TextAlignment::LEFT);
        mLabel->SetTextVAlignment(TextVerticalAlignment::CENTER);
        mLabel->SetOverflow(TextOverflow::TRUNCATE);
        AddChild(mLabel);
    }

    void Dropdown::Destroy()
    {
        if(mOpen)
        {
            mOpen = false;
            GetManager().CloseOverlay(this);
        }
        if(mPopupRoot)
            mPopupRoot->Destroy();
        mPopupRoot = nullptr;
        mPopupScroll = nullptr;
        mPopupItems = nullptr;
        mLabel = nullptr;
        Widget::Destroy();
    }

    void Dropdown::SetFont(Ref<Font> font)
    {
        mFont = font;
        if(mLabel)
            mLabel->SetFont(font);
        mPopupDirty = true;
    }

    void Dropdown::SetOptions(const Vector<String>& options)
    {
        if(mOpen)
            Close();
        mOptions = options;
        if(mSelectedIndex >= static_cast<int>(mOptions.size()))
            mSelectedIndex = mOptions.empty() ? -1 : static_cast<int>(mOptions.size()) - 1;
        mPopupDirty = true;
        UpdateLabel();
    }

    void Dropdown::SetSelectedIndex(int index, bool notify)
    {
        index = (index >= 0 && index < static_cast<int>(mOptions.size())) ? index : -1;
        if(index == mSelectedIndex)
            return;

        mSelectedIndex = index;
        mPopupDirty = true; // Selected highlight
        UpdateLabel();
        if(notify)
            NotifyValueChanged();
    }

    void Dropdown::UpdateLabel()
    {
        if(!mLabel)
            return;

        const bool hasSelection = mSelectedIndex >= 0;
        mLabel->SetText(hasSelection ? mOptions[mSelectedIndex] : Placeholder);
        glm::vec4 color = TextColor;
        if(!hasSelection)
            color.a *= 0.5f;
        mLabel->SetColor(color);
    }

    void Dropdown::LayoutChildren()
    {
        const float padding = TextPadding * mLayoutScale;
        const float arrowArea = mGlobalSize.y * 0.6f;
        for(auto& child : mChildren)
        {
            if(child.get() == mLabel.get())
            {
                mLabel->SetFontSize(FontSize);
                child->LayoutAt({ mGlobalPos.x + padding, mGlobalPos.y }, { glm::max(mGlobalSize.x - padding - arrowArea, 0.0f), mGlobalSize.y }, mLayoutScale);
            }
            else
                child->UpdateLayout(mGlobalPos, mGlobalSize, mLayoutScale);
        }
    }

    void Dropdown::GenerateDrawCommands(DrawList& drawList)
    {
        glm::vec4 tint = mColor;
        if(mIsPressed)      tint *= glm::vec4(0.85f, 0.85f, 0.85f, 1.0f);
        else if(mIsHovered) tint *= glm::vec4(1.08f, 1.08f, 1.08f, 1.0f);
        DrawImage(drawList, Background, BackgroundColor * tint, mGlobalPos, mGlobalSize, mLayoutScale);

        // Arrow: three stacked bars narrowing downwards (a triangle without rotated quads), flipped while open
        const float arrowWidth = glm::min(mGlobalSize.y * 0.28f, 28.0f * mLayoutScale);
        const float barHeight = arrowWidth / 6.0f;
        const glm::vec2 arrowCenter = { mGlobalPos.x + mGlobalSize.x - mGlobalSize.y * 0.35f, mGlobalPos.y + mGlobalSize.y * 0.5f };
        for(int i = 0; i < 3; i++)
        {
            const float width = arrowWidth * (1.0f - static_cast<float>(i) / 3.0f);
            const int row = mOpen ? 2 - i : i;
            const float y = arrowCenter.y + (static_cast<float>(row) - 1.5f) * barHeight;
            DrawRect(drawList, { arrowCenter.x - width * 0.5f, y }, { arrowCenter.x + width * 0.5f, y + barHeight }, TextColor * mColor);
        }

        GenerateChildDrawCommands(drawList);
    }

    void Dropdown::OnPointerUp(const glm::vec2& pos)
    {
        if(mIsPressed && ContainsPoint(pos.x, pos.y))
            mOpen ? Close() : Open();
        Widget::OnPointerUp(pos);
    }

    void Dropdown::RebuildPopup()
    {
        if(mPopupRoot)
            mPopupRoot->Destroy();

        mPopupRoot = Ref<Widget>::Create();
        mPopupScroll = Ref<ScrollView>::Create();
        mPopupScroll->SetAnchor(0.0f, 0.0f);
        mPopupScroll->SetPivot(0.0f, 0.0f);
        mPopupRoot->AddChild(mPopupScroll);

        mPopupItems = Ref<LayoutGroup>::Create();
        mPopupItems->Direction = LayoutDirection::VERTICAL;
        mPopupItems->Spacing = { 0.0f, 0.0f };
        mPopupItems->ControlChildWidth = true;
        mPopupItems->FitContent = true;
        mPopupItems->SetAnchorMin(0.0f, 0.0f);
        mPopupItems->SetAnchorMax(1.0f, 0.0f);
        mPopupItems->SetPivot(0.0f, 0.0f);
        mPopupItems->SetSize(0.0f, 0.0f);
        mPopupScroll->AddChild(mPopupItems);

        const float itemHeight = mLayoutScale > 0.0f ? mGlobalSize.y / mLayoutScale : mSize.y;
        for(int i = 0; i < static_cast<int>(mOptions.size()); i++)
        {
            Ref<Button> item = Ref<Button>::Create(mOptions[i], mFont);
            item->SetSize(0.0f, itemHeight);
            item->NormalColor = i == mSelectedIndex ? SelectedItemColor : glm::vec4(1.0f, 1.0f, 1.0f, 0.0f);
            item->HoverColor = i == mSelectedIndex ? SelectedItemColor : ItemHoverColor;
            item->PressedColor = SelectedItemColor;
            item->GetStyle().CornerRadius = PopupStyle.CornerRadius * 0.5f;
            item->SetTextAlignment(TextAlignment::LEFT, TextPadding);
            item->GetTextWidget()->SetFontSize(FontSize);
            item->GetTextWidget()->SetColor(TextColor);
            item->SetOnClick(Ref<DropdownItemCallback>::Create(this, i));
            mPopupItems->AddChild(item);
        }
        mPopupDirty = false;
    }

    void Dropdown::Open()
    {
        if(mOpen || mOptions.empty())
            return;

        if(mPopupDirty || !mPopupRoot)
            RebuildPopup();
        mPopupScroll->SetScrollPosition({ 0.0f, 0.0f });

        mOpen = true;
        GetManager().OpenOverlay(Ref<Widget>(this));
    }

    void Dropdown::Close()
    {
        if(!mOpen)
            return;

        mOpen = false;
        GetManager().CloseOverlay(this);
    }

    void Dropdown::GenerateOverlayDrawCommands(DrawList& drawList)
    {
        if(!mPopupRoot)
            return;

        // Below the field when it fits on screen, above otherwise, one item = the field height
        const FrameInfo& frame = GetFrameInfo();
        const float itemHeight = mGlobalSize.y;
        const int visibleItems = glm::clamp(static_cast<int>(mOptions.size()), 1, glm::max(MaxVisibleItems, 1));
        const float listHeight = itemHeight * static_cast<float>(visibleItems);
        const float below = mGlobalPos.y + mGlobalSize.y;
        const bool fitsBelow = below + listHeight <= frame.ScreenSize.y || mGlobalPos.y - listHeight < 0.0f;
        const glm::vec2 listPos = { mGlobalPos.x, fitsBelow ? below : mGlobalPos.y - listHeight };
        const glm::vec2 listSize = { mGlobalSize.x, listHeight };

        const float scale = mLayoutScale > 0.0f ? mLayoutScale : 1.0f;
        mPopupScroll->SetOffset(listPos.x / scale, listPos.y / scale);
        mPopupScroll->SetSize(listSize.x / scale, listSize.y / scale);
        mPopupRoot->LayoutAsRoot(frame.ScreenSize, scale);

        DrawImage(drawList, PopupStyle, PopupColor * mColor, listPos, listSize, scale);
        mPopupRoot->GenerateDrawCommands(drawList);
    }

    Widget* Dropdown::OverlayHitTest(float x, float y)
    {
        return mPopupRoot ? mPopupRoot->HitTest(x, y) : nullptr; // nullptr = outside, the Manager closes the list
    }
}
