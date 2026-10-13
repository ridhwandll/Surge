// Copyright (c) - SurgeTechnologies - All rights reserved
#include "UIWidgets.hpp"
#include "Surge/Core/Core.hpp"
#include "Surge/Graphics/Renderer/Renderer.hpp"
#include "Surge/Asset/AssetManager.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

namespace Surge
{
    static UI::FrameInfo sFrameInfo;

    UI::FrameInfo& UI::GetFrameInfo()
    {
        return sFrameInfo;
    }

    // DrawList
    bool UI::DrawList::IsClippedOut(const glm::vec2& min, const glm::vec2& max) const
    {
        if(CurrentClipIndex == UI_NO_CLIP)
            return false;

        const glm::vec4& clip = ClipRects[CurrentClipIndex - 1];
        return max.x <= clip.x || min.x >= clip.z || max.y <= clip.y || min.y >= clip.w;
    }

    void UI::DrawList::AddSprite(const UISpriteCmd& cmd)
    {
        if(IsClippedOut(cmd.Min, cmd.Max))
            return; // Off screen list items cost nothing past this point

        Order.push_back(UIDrawItem { .IsText = false, .ClipIndex = CurrentClipIndex, .Index = static_cast<Uint>(Sprites.size()) });
        Sprites.push_back(cmd);
    }

    void UI::DrawList::AddText(const TextSubmitCmd& cmd)
    {
        Order.push_back(UIDrawItem { .IsText = true, .ClipIndex = CurrentClipIndex, .Index = static_cast<Uint>(Texts.size()) });
        Texts.push_back(cmd);
    }

    uint16_t UI::DrawList::PushClip(const glm::vec2& min, const glm::vec2& max)
    {
        const uint16_t previous = CurrentClipIndex;

        if(ClipRects.size() >= (UINT16_MAX - 1))
            return previous; // Out of indices, keep the current clip

        glm::vec4 rect = { min.x, min.y, max.x, max.y };
        if(CurrentClipIndex != UI_NO_CLIP)
        {
            const glm::vec4& parent = ClipRects[CurrentClipIndex - 1];
            rect = { glm::max(rect.x, parent.x), glm::max(rect.y, parent.y), glm::min(rect.z, parent.z), glm::min(rect.w, parent.w) };
        }

        ClipRects.push_back(rect);
        CurrentClipIndex = static_cast<uint16_t>(ClipRects.size()); // Index + 1
        return previous;
    }

    // -----------------------------------------------------------------------------------------------
    // Image drawing
    // -----------------------------------------------------------------------------------------------

    void UI::ImageStyle::SetTexture(ImageHandle texture, Ref<Asset> textureAsset)
    {
        Texture = texture;
        TextureAsset = textureAsset;
        TextureSize = { 0.0f, 0.0f };
        if(!texture.IsNull())
        {
            const ImageDesc& desc = Core::GetRenderer()->GetRHI()->GetDesc(texture);
            TextureSize = { static_cast<float>(desc.Width), static_cast<float>(desc.Height) };
        }
    }

    void UI::DrawRect(DrawList& drawList, const glm::vec2& min, const glm::vec2& max, const glm::vec4& color, float radiusPixels)
    {
        UISpriteCmd cmd;
        cmd.Min = min;
        cmd.Max = max;
        cmd.Color = color;
        cmd.Texture = ImageHandle::Invalid();
        cmd.CornerRadius = radiusPixels;
        drawList.AddSprite(cmd);
    }

    static void DrawImageInternal(UI::DrawList& drawList, const UI::ImageStyle& style, const glm::vec4& color, const glm::vec2& pos, const glm::vec2& size, float scale, UI::ImageMode mode, float fillAmount, UI::FillOrigin origin)
    {
        if(size.x <= 0.0f || size.y <= 0.0f)
            return;

        glm::vec2 min = pos;
        glm::vec2 max = pos + size;

        // Fit the texture aspect inside the rect, centered
        if(style.PreserveAspect && style.TextureSize.x > 0.0f && style.TextureSize.y > 0.0f)
        {
            const float textureAspect = style.TextureSize.x / style.TextureSize.y;
            if(size.x / size.y > textureAspect)
            {
                const float width = size.y * textureAspect;
                min.x += (size.x - width) * 0.5f;
                max.x = min.x + width;
            }
            else
            {
                const float height = size.x / textureAspect;
                min.y += (size.y - height) * 0.5f;
                max.y = min.y + height;
            }
        }

        float radius = mode == UI::ImageMode::SLICED ? 0.0f : style.CornerRadius * scale;

        // Border: a rect behind the part, the part shrinks by the border width (inner radius = outer radius - width keeps the border even)
        if(style.BorderWidth > 0.0f && style.BorderColor.a > 0.0f)
        {
            const float borderWidth = style.BorderWidth * scale;
            UI::DrawRect(drawList, min, max, style.BorderColor * glm::vec4(1.0f, 1.0f, 1.0f, color.a), radius);

            min += borderWidth; // (Rid) Adding a +ve number moves X to the right and Y downward
            max -= borderWidth; // Subtracting a +ve number moves X to the left and Y upward

            radius = glm::max(radius - borderWidth, 0.0f);
            if(max.x <= min.x || max.y <= min.y)
                return;
        }

        if(color.a <= 0.0f)
            return;

        UISpriteCmd cmd;
        cmd.Color = color;
        cmd.Texture = style.Texture;
        cmd.CornerRadius = radius;

        switch(mode)
        {
            case UI::ImageMode::SIMPLE:
            {
                cmd.Min = min;
                cmd.Max = max;
                drawList.AddSprite(cmd);
                break;
            }

            case UI::ImageMode::FILLED:
            {
                const float amount = glm::clamp(fillAmount, 0.0f, 1.0f);
                if(amount <= 0.0f)
                    return;

                // Textured: crop through the UVs so the texture is not squashed (rounded corners then stay those of the full rect).
                // Plain color: shrink the rect with full UVs, so rounded ends follow the fill (pill shaped bars)
                const bool crop = !style.Texture.IsNull();
                const glm::vec2 extent = max - min;
                cmd.Min = min;
                cmd.Max = max;
                switch(origin)
                {
                    case UI::FillOrigin::LEFT:
                        cmd.Max.x = min.x + extent.x * amount;
                        if(crop) cmd.UVMax.x = amount;
                        break;
                    case UI::FillOrigin::RIGHT:
                        cmd.Min.x = max.x - extent.x * amount;
                        if(crop) cmd.UVMin.x = 1.0f - amount;
                        break;
                    case UI::FillOrigin::TOP:
                        cmd.Max.y = min.y + extent.y * amount;
                        if(crop) cmd.UVMax.y = amount;
                        break;
                    case UI::FillOrigin::BOTTOM:
                        cmd.Min.y = max.y - extent.y * amount;
                        if(crop) cmd.UVMin.y = 1.0f - amount;
                        break;
                }
                drawList.AddSprite(cmd);
                break;
            }

            case UI::ImageMode::SLICED:
            {
                if(style.TextureSize.x <= 0.0f || style.TextureSize.y <= 0.0f)
                {
                    cmd.Min = min;
                    cmd.Max = max;
                    drawList.AddSprite(cmd);
                    break;
                }

                // Borders in pixels, scaled down together when they don't fit (a 9-slice smaller than its corners)
                glm::vec4 border = style.SliceBorder * style.SliceScale * scale;
                const glm::vec2 extent = max - min;
                if(border.x + border.z > extent.x && border.x + border.z > 0.0f)
                {
                    const float fit = extent.x / (border.x + border.z);
                    border.x *= fit;
                    border.z *= fit;
                }
                if(border.y + border.w > extent.y && border.y + border.w > 0.0f)
                {
                    const float fit = extent.y / (border.y + border.w);
                    border.y *= fit;
                    border.w *= fit;
                }

                const float xs[4] = { min.x, min.x + border.x, max.x - border.z, max.x };
                const float ys[4] = { min.y, min.y + border.y, max.y - border.w, max.y };
                const float us[4] = { 0.0f, style.SliceBorder.x / style.TextureSize.x, 1.0f - style.SliceBorder.z / style.TextureSize.x, 1.0f };
                const float vs[4] = { 0.0f, style.SliceBorder.y / style.TextureSize.y, 1.0f - style.SliceBorder.w / style.TextureSize.y, 1.0f };

                cmd.CornerRadius = 0.0f;
                for(int row = 0; row < 3; row++)
                {
                    for(int column = 0; column < 3; column++)
                    {
                        if(!style.FillCenter && row == 1 && column == 1)
                            continue;
                        if(xs[column + 1] <= xs[column] || ys[row + 1] <= ys[row])
                            continue; // Zero border on that side

                        cmd.Min = { xs[column], ys[row] };
                        cmd.Max = { xs[column + 1], ys[row + 1] };
                        cmd.UVMin = { us[column], vs[row] };
                        cmd.UVMax = { us[column + 1], vs[row + 1] };
                        drawList.AddSprite(cmd);
                    }
                }
                break;
            }
        }
    }

    void UI::DrawImage(DrawList& drawList, const ImageStyle& style, const glm::vec4& color, const glm::vec2& pos, const glm::vec2& size, float scale)
    {
        DrawImageInternal(drawList, style, color, pos, size, scale, style.Mode, style.FillAmount, style.Origin);
    }

    void UI::DrawFilledImage(DrawList& drawList, const ImageStyle& style, const glm::vec4& color, const glm::vec2& pos, const glm::vec2& size, float scale, float amount, FillOrigin origin)
    {
        if(style.Mode != ImageMode::SLICED)
        {
            DrawImageInternal(drawList, style, color, pos, size, scale, ImageMode::FILLED, amount, origin);
            return;
        }

        // A 9-slice stretches cleanly, so it is resized instead of cropped
        amount = glm::clamp(amount, 0.0f, 1.0f);
        glm::vec2 fillPos = pos;
        glm::vec2 fillSize = size;
        switch(origin)
        {
            case FillOrigin::LEFT:   fillSize.x *= amount; break;
            case FillOrigin::RIGHT:  fillSize.x *= amount; fillPos.x += size.x - fillSize.x; break;
            case FillOrigin::TOP:    fillSize.y *= amount; break;
            case FillOrigin::BOTTOM: fillSize.y *= amount; fillPos.y += size.y - fillSize.y; break;
        }
        DrawImageInternal(drawList, style, color, fillPos, fillSize, scale, ImageMode::SLICED, 1.0f, origin);
    }

    float UI::MeasureText(const Font* font, std::string_view text)
    {
        if(!font)
            return 0.0f;

        // Must mirror UIOverlayPass::AppendText: chars are looked up as-is (bytes >= 0x80 have no glyph), kerning with the previous drawn glyph
        float width = 0.0f;
        Uint previous = 0;
        for(const char c : text)
        {
            if(c == '\n')
                break;

            const FontGlyph* glyph = font->GetGlyph(c);
            if(!glyph)
                continue;

            if(previous != 0)
                width += font->GetKerning(previous, c);

            width += glyph->Advance;
            previous = c;
        }
        return width;
    }

    // -----------------------------------------------------------------------------------------------
    // Widget
    // -----------------------------------------------------------------------------------------------

    void UI::Widget::AddChild(Ref<Widget> child)
    {
        if(!child || child.get() == this)
            return;

        if(child->mParent == this)
            return; // Already my baby

        if(child->mParent)
            child->mParent->RemoveChild(child.get());

        child->mParent = this;
        mChildren.push_back(child);
    }

    void UI::Widget::RemoveChild(Widget* child)
    {
        for(auto it = mChildren.begin(); it != mChildren.end(); ++it)
        {
            if(it->get() == child)
            {
                child->mParent = nullptr;
                mChildren.erase(it); // May release the last reference to child
                return;
            }
        }
    }

    UI::Widget* UI::Widget::FindChild(const String& name, bool recursive)
    {
        for(auto& child : mChildren)
        {
            if(child->mName == name)
                return child.get();
        }

        if(recursive)
        {
            for(auto& child : mChildren)
            {
                if(Widget* found = child->FindChild(name, true))
                    return found;
            }
        }
        return nullptr;
    }

    void UI::Widget::LayoutAsRoot(const glm::vec2& screenSize, float scale)
    {
        mLayoutScale = scale;
        mGlobalPos = { 0.0f, 0.0f };
        mGlobalSize = screenSize;
        LayoutChildren();
    }

    void UI::Widget::ComputeRect(const glm::vec2& parentPos, const glm::vec2& parentSize, float scale)
    {
        mLayoutScale = scale;

        const glm::vec2 anchorSpan = mAnchorMax - mAnchorMin;
        mGlobalSize = glm::max(parentSize * anchorSpan + mSize * scale, glm::vec2(0.0f));

        const glm::vec2 pivotPoint = parentPos + parentSize * (mAnchorMin + anchorSpan * mPivot) + mLocalOffset * scale;
        mGlobalPos = pivotPoint - mGlobalSize * mPivot;
    }

    void UI::Widget::UpdateLayout(const glm::vec2& parentPos, const glm::vec2& parentSize, float scale)
    {
        ComputeRect(parentPos, parentSize, scale);
        LayoutChildren();
    }

    void UI::Widget::LayoutAt(const glm::vec2& pos, const glm::vec2& size, float scale)
    {
        mLayoutScale = scale;
        mGlobalPos = pos;
        mGlobalSize = glm::max(size, glm::vec2(0.0f));
        LayoutChildren();
    }

    void UI::Widget::LayoutChildren()
    {
        for(auto& child : mChildren)
            child->UpdateLayout(mGlobalPos, mGlobalSize, mLayoutScale);
    }

    void UI::Widget::TranslateLayout(const glm::vec2& delta)
    {
        mGlobalPos += delta;
        for(auto& child : mChildren)
            child->TranslateLayout(delta);
    }

    UI::Widget* UI::Widget::HitTestChildren(float x, float y)
    {
        // Children drawn later sit on top, test them first
        for(auto it = mChildren.rbegin(); it != mChildren.rend(); ++it)
        {
            if(Widget* hit = (*it)->HitTest(x, y))
                return hit;
        }
        return nullptr;
    }

    UI::Widget* UI::Widget::HitTest(float x, float y)
    {
        if(!mVisible)
            return nullptr;

        // Children first (they draw on top of parents)
        if(Widget* hit = HitTestChildren(x, y))
            return hit;

        if(!mIsInteractable)
            return nullptr;

        return ContainsPoint(x, y) ? this : nullptr;
    }

    void UI::Widget::Destroy()
    {
        mOnClick = nullptr;
        mOnHoverEnter = nullptr;
        mOnHoverExit = nullptr;
        mOnValueChanged = nullptr;

        for(auto& child : mChildren)
        {
            child->Destroy();
            child->mParent = nullptr;
        }

        mChildren.clear();
        mParent = nullptr;
    }

    // -----------------------------------------------------------------------------------------------
    // Text
    // -----------------------------------------------------------------------------------------------

    UI::Text::Text(const String& text, AssetID fontAsset)
        : mText(text)
    {
        mSize = { 0.0f, 0.0f };
        if(fontAsset)
            mFontAsset = Core::GetAssetManager()->Load<Font>(fontAsset);
    }

    UI::Text::Text(const String& text, Ref<Font> font)
        : mText(text), mFontAsset(font)
    {
        mSize = { 0.0f, 0.0f };
    }

    const String& UI::Text::GetDisplayText(float maxWidthEm)
    {
        const Font* font = mFontAsset.Raw();
        if(mTruncatedWidth == maxWidthEm && mTruncatedFont == font && mTruncatedSource == mText)
            return mTruncatedText;

        mTruncatedSource = mText;
        mTruncatedWidth = maxWidthEm;
        mTruncatedFont = font;

        const size_t lineEnd = mText.find('\n');
        const std::string_view line(mText.data(), lineEnd == String::npos ? mText.size() : lineEnd);
        if(lineEnd == String::npos && MeasureText(font, line) <= maxWidthEm)
        {
            mTruncatedText = mText;
            return mTruncatedText;
        }

        // Longest prefix that still fits together with the ellipsis, one forward walk (same advance/kerning rules as MeasureText)
        const float ellipsisWidth = MeasureText(font, "...");
        float width = 0.0f;
        Uint previous = 0;
        size_t fitCount = 0;
        for(size_t i = 0; i < line.size(); i++)
        {
            const char c = line[i];
            if(const FontGlyph* glyph = font ? font->GetGlyph(c) : nullptr)
            {
                width += (previous != 0 ? font->GetKerning(previous, c) : 0.0f) + glyph->Advance;
                previous = c;
            }
            if(width + ellipsisWidth > maxWidthEm)
                break;
            fitCount = i + 1;
        }

        // Never cut a UTF-8 sequence in half, drop trailing spaces before the dots
        while(fitCount > 0 && fitCount < line.size() && (static_cast<unsigned char>(line[fitCount]) & 0xC0) == 0x80)
            fitCount--;
        while(fitCount > 0 && line[fitCount - 1] == ' ')
            fitCount--;

        mTruncatedText.assign(line.data(), fitCount);
        mTruncatedText += "...";
        return mTruncatedText;
    }

    void UI::Text::GenerateDrawCommands(DrawList& drawList)
    {
        const float fontSize = mFontSize * mLayoutScale;

        // Text origin on the rect edge picked by the alignment (with a zero sized rect this is the anchor point itself)
        glm::vec2 origin = mGlobalPos;
        if(mTextAlignment == TextAlignment::CENTER)     origin.x += mGlobalSize.x * 0.5f;
        else if(mTextAlignment == TextAlignment::RIGHT) origin.x += mGlobalSize.x;

        if(mTextVerticalAlignment == TextVerticalAlignment::CENTER)
            origin.y += mGlobalSize.y * 0.5f;
        else if(mTextVerticalAlignment == TextVerticalAlignment::BOTTOM || mTextVerticalAlignment == TextVerticalAlignment::BASELINE)
            origin.y += mGlobalSize.y;

        glm::mat4 transform = glm::mat4(1.0f);
        transform = glm::translate(transform, glm::vec3(origin.x, origin.y, 0.0f));
        transform = glm::scale(transform, glm::vec3(fontSize, fontSize, 1.0f));

        const bool hasWidth = mGlobalSize.x > 0.0f && fontSize > 0.0f;

        TextSubmitCmd cmd {};
        cmd.Transform = transform;
        cmd.Text = (mOverflow == TextOverflow::TRUNCATE && hasWidth && mFontAsset) ? GetDisplayText(mGlobalSize.x / fontSize) : mText;
        cmd.Color = mColor;
        cmd.FontAsset = mFontAsset.Raw();
        cmd.Billboard = false;
        cmd.Alignment = mTextAlignment;
        cmd.VerticalAlignment = mTextVerticalAlignment;

        // Glyph layout happens in font (em) units, the transform scales them by fontSize, so the pixel width must be converted
        cmd.MaxWidth = (mOverflow == TextOverflow::WRAP && hasWidth) ? mGlobalSize.x / fontSize : 0.0f;

        drawList.AddText(cmd);
        GenerateChildDrawCommands(drawList);
    }

    // -----------------------------------------------------------------------------------------------
    // Button
    // -----------------------------------------------------------------------------------------------

    void UI::Button::Init(Ref<Text> textWidget)
    {
        mIsInteractable = true;
        SetSize(200.0f, 50.0f);

        mTextWidget = textWidget;
        mTextWidget->SetFontSize(18.0f);
        mTextWidget->SetSize(0.0f, 0.0f);
        AddChild(mTextWidget);

        SetTextAlignment(TextAlignment::CENTER);
        SetTextVAlignment(TextVerticalAlignment::CENTER);
    }

    void UI::Button::SetTextAlignment(TextAlignment align, float paddingX)
    {
        switch(align)
        {
            case TextAlignment::CENTER:
                mTextWidget->SetAnchor(0.5f, mTextWidget->GetAnchor().y);
                mTextWidget->SetOffset(0.0f, mTextWidget->GetOffset().y);
                break;
            case TextAlignment::LEFT:
                mTextWidget->SetAnchor(0.0f, mTextWidget->GetAnchor().y);
                mTextWidget->SetOffset(paddingX, mTextWidget->GetOffset().y);
                break;
            case TextAlignment::RIGHT:
                mTextWidget->SetAnchor(1.0f, mTextWidget->GetAnchor().y);
                mTextWidget->SetOffset(-paddingX, mTextWidget->GetOffset().y);
                break;
        }
        mTextWidget->SetPivot(0.0f, 0.0f);
        mTextWidget->SetTextAlignment(align);
    }

    void UI::Button::SetTextVAlignment(TextVerticalAlignment alignment, float paddingY)
    {
        switch(alignment)
        {
            case TextVerticalAlignment::CENTER:
                mTextWidget->SetAnchor(mTextWidget->GetAnchor().x, 0.5f);
                mTextWidget->SetOffset(mTextWidget->GetOffset().x, 0.0f);
                break;
            case TextVerticalAlignment::TOP:
                mTextWidget->SetAnchor(mTextWidget->GetAnchor().x, 0.0f);
                mTextWidget->SetOffset(mTextWidget->GetOffset().x, paddingY);
                break;
            case TextVerticalAlignment::BOTTOM:
            case TextVerticalAlignment::BASELINE:
                mTextWidget->SetAnchor(mTextWidget->GetAnchor().x, 1.0f);
                mTextWidget->SetOffset(mTextWidget->GetOffset().x, -paddingY);
                break;
        }
        mTextWidget->SetPivot(0.0f, 0.0f);
        mTextWidget->SetTextVAlignment(alignment);
    }

    // -----------------------------------------------------------------------------------------------
    // ProgressBar
    // -----------------------------------------------------------------------------------------------

    UI::ProgressBar::ProgressBar()
    {
        mSize = { 400.0f, 40.0f };
    }

    void UI::ProgressBar::GenerateDrawCommands(DrawList& drawList)
    {
        DrawImage(drawList, Background, BackgroundColor * mColor, mGlobalPos, mGlobalSize, mLayoutScale);

        const float padding = FillPadding * mLayoutScale;
        const glm::vec2 fillPos = mGlobalPos + padding;
        const glm::vec2 fillSize = mGlobalSize - padding * 2.0f;
        const float amount = GetNormalizedValue();
        if(amount > 0.0f && fillSize.x > 0.0f && fillSize.y > 0.0f)
            DrawFilledImage(drawList, Fill, FillColor * mColor, fillPos, fillSize, mLayoutScale, amount, Direction);

        GenerateChildDrawCommands(drawList);
    }

    // -----------------------------------------------------------------------------------------------
    // Toggle
    // -----------------------------------------------------------------------------------------------

    UI::Toggle::Toggle(const String& label, Ref<Font> font)
    {
        mIsInteractable = true;
        mSize = { 320.0f, 60.0f };
        Box.CornerRadius = 10.0f;
        Check.CornerRadius = 5.0f;
        Knob.CornerRadius = 10000.0f; // Clamped to half the size in the shader: a circle

        mLabel = Ref<Text>::Create(label, font);
        mLabel->SetFontSize(30.0f);
        mLabel->SetTextAlignment(TextAlignment::LEFT);
        mLabel->SetTextVAlignment(TextVerticalAlignment::CENTER);
        mLabel->SetOverflow(TextOverflow::TRUNCATE);
        AddChild(mLabel);
    }

    void UI::Toggle::Destroy()
    {
        Widget::Destroy();
        mLabel = nullptr;
    }

    void UI::Toggle::GetBoxRect(glm::vec2& outPos, glm::vec2& outSize) const
    {
        const float height = mGlobalSize.y;
        const float width = Style == ToggleStyle::SWITCH ? height * 1.8f : height;
        outPos = mGlobalPos;
        outSize = { glm::min(width, mGlobalSize.x), height };
    }

    void UI::Toggle::LayoutChildren()
    {
        glm::vec2 boxPos, boxSize;
        GetBoxRect(boxPos, boxSize);
        const float gap = 14.0f * mLayoutScale;

        for(auto& child : mChildren)
        {
            if(child.get() == mLabel.get())
                child->LayoutAt({ boxPos.x + boxSize.x + gap, mGlobalPos.y }, { mGlobalSize.x - boxSize.x - gap, mGlobalSize.y }, mLayoutScale);
            else
                child->UpdateLayout(mGlobalPos, mGlobalSize, mLayoutScale);
        }
    }

    void UI::Toggle::SetOn(bool on, bool notify)
    {
        if(!notify)
            mKnobT = on ? 1.0f : 0.0f; // Loading a layout: no animation

        if(mIsOn == on)
            return;

        // Radio behaviour: grouped siblings turn off first (from taps and from code), so callbacks see a consistent group
        if(on && !Group.empty() && mParent)
        {
            for(auto& sibling : mParent->GetChildren())
            {
                if(sibling.get() != this && sibling->GetType() == WidgetType::TOGGLE)
                {
                    Toggle* other = static_cast<Toggle*>(sibling.get());
                    if(other->Group == Group)
                        other->SetOn(false, notify);
                }
            }
        }

        mIsOn = on;
        if(notify)
            NotifyValueChanged();
    }

    void UI::Toggle::OnPointerUp(const glm::vec2& pos)
    {
        if(mIsPressed)
        {
            // In a group a tap only turns the toggle on, unless switching the active one off is allowed
            if(Group.empty() || !mIsOn || AllowSwitchOff)
                SetOn(!mIsOn);
        }
        Widget::OnPointerUp(pos);
    }

    void UI::Toggle::GenerateDrawCommands(DrawList& drawList)
    {
        // Knob slides / check fades towards the state, ~0.12 s
        const float target = mIsOn ? 1.0f : 0.0f;
        const float step = GetFrameInfo().DeltaTime * 8.0f;
        mKnobT = mKnobT < target ? glm::min(mKnobT + step, target) : glm::max(mKnobT - step, target);

        glm::vec2 boxPos, boxSize;
        GetBoxRect(boxPos, boxSize);
        const glm::vec4 pressTint = mIsPressed ? glm::vec4(0.85f, 0.85f, 0.85f, 1.0f) : glm::vec4(1.0f);

        if(Style == ToggleStyle::CHECKBOX)
        {
            DrawImage(drawList, Box, BoxColor * mColor * pressTint, boxPos, boxSize, mLayoutScale);
            if(mKnobT > 0.0f)
            {
                const glm::vec2 inset = boxSize * 0.22f;
                glm::vec4 checkColor = CheckColor * mColor;
                checkColor.a *= mKnobT;
                DrawImage(drawList, Check, checkColor, boxPos + inset, boxSize - inset * 2.0f, mLayoutScale);
            }
        }
        else
        {
            // Track blends from the off (Box) to the on (Check) color, both drawn with the Box style (pill by default)
            DrawImage(drawList, Box, glm::mix(BoxColor, CheckColor, mKnobT) * mColor * pressTint, boxPos, boxSize, mLayoutScale);

            const float inset = boxSize.y * 0.1f;
            const float knobSize = boxSize.y - inset * 2.0f;
            const float travel = glm::max(boxSize.x - inset * 2.0f - knobSize, 0.0f);
            const glm::vec2 knobPos = { boxPos.x + inset + travel * mKnobT, boxPos.y + inset };
            DrawImage(drawList, Knob, KnobColor * mColor, knobPos, { knobSize, knobSize }, mLayoutScale);
        }

        GenerateChildDrawCommands(drawList);
    }

    // -----------------------------------------------------------------------------------------------
    // Slider
    // -----------------------------------------------------------------------------------------------

    UI::Slider::Slider()
    {
        mIsInteractable = true;
        mSize = { 400.0f, 50.0f };
        Track.CornerRadius = 10000.0f;
        Fill.CornerRadius = 10000.0f;
        Handle.CornerRadius = 10000.0f;
    }

    void UI::Slider::SetValue(float value, bool notify)
    {
        value = glm::clamp(value, glm::min(Min, Max), glm::max(Min, Max));
        if(WholeNumbers)
            value = std::round(value);
        if(value == mValue)
            return;

        mValue = value;
        if(notify)
            NotifyValueChanged();
    }

    void UI::Slider::SetValueFromPointer(const glm::vec2& pos)
    {
        // The handle center travels between half a handle from each end
        const float handle = HandleSize * mLayoutScale;
        float t;
        if(IsHorizontal())
        {
            const float travel = glm::max(mGlobalSize.x - handle, 1.0f);
            t = (pos.x - (mGlobalPos.x + handle * 0.5f)) / travel;
        }
        else
        {
            const float travel = glm::max(mGlobalSize.y - handle, 1.0f);
            t = (pos.y - (mGlobalPos.y + handle * 0.5f)) / travel;
        }

        t = glm::clamp(t, 0.0f, 1.0f);
        if(Direction == FillOrigin::RIGHT || Direction == FillOrigin::BOTTOM)
            t = 1.0f - t;
        SetValue(Min + t * (Max - Min));
    }

    void UI::Slider::OnPointerDown(const glm::vec2& pos)
    {
        mIsPressed = true;
        mDragging = true;
        SetValueFromPointer(pos);
    }

    void UI::Slider::OnPointerDrag(const glm::vec2& pos)
    {
        if(mDragging)
            SetValueFromPointer(pos);
    }

    void UI::Slider::OnPointerUp(const glm::vec2& pos)
    {
        mDragging = false;
        Widget::OnPointerUp(pos);
    }

    void UI::Slider::GenerateDrawCommands(DrawList& drawList)
    {
        const bool horizontal = IsHorizontal();
        const float handle = glm::min(HandleSize * mLayoutScale, horizontal ? mGlobalSize.x : mGlobalSize.y);
        const float t = GetNormalizedValue();
        const bool reversed = Direction == FillOrigin::RIGHT || Direction == FillOrigin::BOTTOM;
        const float position = reversed ? 1.0f - t : t;

        glm::vec2 trackPos = mGlobalPos;
        glm::vec2 trackSize = mGlobalSize;
        glm::vec2 handleCenter;
        if(horizontal)
        {
            const float thickness = TrackThickness > 0.0f ? glm::min(TrackThickness * mLayoutScale, mGlobalSize.y) : mGlobalSize.y;
            trackPos.y += (mGlobalSize.y - thickness) * 0.5f;
            trackSize.y = thickness;
            handleCenter = { mGlobalPos.x + handle * 0.5f + (mGlobalSize.x - handle) * position, mGlobalPos.y + mGlobalSize.y * 0.5f };
        }
        else
        {
            const float thickness = TrackThickness > 0.0f ? glm::min(TrackThickness * mLayoutScale, mGlobalSize.x) : mGlobalSize.x;
            trackPos.x += (mGlobalSize.x - thickness) * 0.5f;
            trackSize.x = thickness;
            handleCenter = { mGlobalPos.x + mGlobalSize.x * 0.5f, mGlobalPos.y + handle * 0.5f + (mGlobalSize.y - handle) * position };
        }

        DrawImage(drawList, Track, TrackColor * mColor, trackPos, trackSize, mLayoutScale);

        // Fill: from the origin edge to the handle center
        glm::vec2 fillPos = trackPos;
        glm::vec2 fillSize = trackSize;
        switch(Direction)
        {
            case FillOrigin::LEFT:   fillSize.x = handleCenter.x - trackPos.x; break;
            case FillOrigin::RIGHT:  fillPos.x = handleCenter.x; fillSize.x = trackPos.x + trackSize.x - handleCenter.x; break;
            case FillOrigin::TOP:    fillSize.y = handleCenter.y - trackPos.y; break;
            case FillOrigin::BOTTOM: fillPos.y = handleCenter.y; fillSize.y = trackPos.y + trackSize.y - handleCenter.y; break;
        }
        DrawImage(drawList, Fill, FillColor * mColor, fillPos, fillSize, mLayoutScale);

        const glm::vec4 handleColor = mIsPressed ? HandleColor * glm::vec4(0.85f, 0.85f, 0.85f, 1.0f) : HandleColor;
        DrawImage(drawList, Handle, handleColor * mColor, handleCenter - handle * 0.5f, { handle, handle }, mLayoutScale);

        GenerateChildDrawCommands(drawList);
    }

    // -----------------------------------------------------------------------------------------------
    // Joystick
    // -----------------------------------------------------------------------------------------------

    UI::Joystick::Joystick()
    {
        mIsInteractable = true;
        mSize = { 300.0f, 300.0f };
        Base.CornerRadius = 100000.0f;
        Knob.CornerRadius = 100000.0f;
    }

    float UI::Joystick::GetBaseRadius() const
    {
        return BaseSize > 0.0f ? BaseSize * mLayoutScale * 0.5f : glm::min(mGlobalSize.x, mGlobalSize.y) * 0.5f;
    }

    void UI::Joystick::OnPointerDown(const glm::vec2& pos)
    {
        mIsPressed = true;
        mActive = true;

        const glm::vec2 rectCenter = mGlobalPos + mGlobalSize * 0.5f;
        if(Mode == JoystickMode::FLOATING)
        {
            // Center under the finger, kept far enough from the edges that the whole base stays inside the rect
            const float radius = GetBaseRadius();
            const glm::vec2 lo = mGlobalPos + radius;
            const glm::vec2 hi = mGlobalPos + mGlobalSize - radius;
            mCenter.x = lo.x <= hi.x ? glm::clamp(pos.x, lo.x, hi.x) : rectCenter.x;
            mCenter.y = lo.y <= hi.y ? glm::clamp(pos.y, lo.y, hi.y) : rectCenter.y;
        }
        else
            mCenter = rectCenter;

        OnPointerDrag(pos);
    }

    void UI::Joystick::OnPointerDrag(const glm::vec2& pos)
    {
        if(!mActive)
            return;

        const float maxDistance = GetBaseRadius() * HandleRange;
        glm::vec2 delta = pos - mCenter;
        const float distance = glm::length(delta);
        if(distance > maxDistance && distance > 0.0f)
            delta *= maxDistance / distance;
        mKnobOffset = delta;

        // Normalized stick, dead zone removed and the rest rescaled to 0..1 so small moves still start at 0
        const glm::vec2 stick = maxDistance > 0.0f ? delta / maxDistance : glm::vec2(0.0f);
        const float magnitude = glm::length(stick);
        const float deadZone = glm::clamp(DeadZone, 0.0f, 0.99f);
        if(magnitude <= deadZone)
            mDirection = { 0.0f, 0.0f };
        else
        {
            const float rescaled = glm::min((magnitude - deadZone) / (1.0f - deadZone), 1.0f);
            const glm::vec2 direction = stick / magnitude * rescaled;
            mDirection = { direction.x, -direction.y }; // UI y grows down, games expect +Y up
        }
    }

    void UI::Joystick::Release()
    {
        mIsPressed = false;
        mActive = false;
        mKnobOffset = { 0.0f, 0.0f };
        mDirection = { 0.0f, 0.0f };
    }

    void UI::Joystick::GenerateDrawCommands(DrawList& drawList)
    {
        if(mActive || !HideWhenIdle)
        {
            const float radius = GetBaseRadius();
            const glm::vec2 center = mActive ? mCenter : mGlobalPos + mGlobalSize * 0.5f;
            DrawImage(drawList, Base, BaseColor * mColor, center - radius, glm::vec2(radius * 2.0f), mLayoutScale);

            const float knobRadius = HandleSize > 0.0f ? HandleSize * mLayoutScale * 0.5f : radius * 0.45f;
            DrawImage(drawList, Knob, KnobColor * mColor, center + mKnobOffset - knobRadius, glm::vec2(knobRadius * 2.0f), mLayoutScale);
        }
        GenerateChildDrawCommands(drawList);
    }

    // -----------------------------------------------------------------------------------------------
    // Spinner
    // -----------------------------------------------------------------------------------------------

    void UI::Spinner::GenerateDrawCommands(DrawList& drawList)
    {
        const int count = glm::clamp(DotCount, 3, 24);
        const float radius = glm::min(mGlobalSize.x, mGlobalSize.y) * 0.5f;
        const float dot = glm::max(radius * 2.0f * glm::clamp(DotSize, 0.02f, 0.5f), 1.0f);
        const float ring = radius - dot * 0.5f;
        const glm::vec2 center = mGlobalPos + mGlobalSize * 0.5f;

        // The head moves clockwise one dot per 1/count revolution, dots fade with their distance behind it
        const float head = glm::fract(GetFrameInfo().Time * Speed) * static_cast<float>(count);
        for(int i = 0; i < count; i++)
        {
            const float angle = glm::two_pi<float>() * static_cast<float>(i) / static_cast<float>(count);
            const glm::vec2 p = center + ring * glm::vec2(std::sin(angle), -std::cos(angle)); // Clockwise from 12 o'clock
            const float behind = std::fmod(head - static_cast<float>(i) + static_cast<float>(count), static_cast<float>(count)) / static_cast<float>(count);

            glm::vec4 color = mColor;
            color.a *= 1.0f - behind * 0.85f;
            DrawRect(drawList, p - dot * 0.5f, p + dot * 0.5f, color, dot * 0.5f);
        }
        GenerateChildDrawCommands(drawList);
    }

    // -----------------------------------------------------------------------------------------------
    // SafeArea
    // -----------------------------------------------------------------------------------------------

    void UI::SafeArea::LayoutChildren()
    {
        const FrameInfo& frame = GetFrameInfo();
        glm::vec2 min = mGlobalPos;
        glm::vec2 max = mGlobalPos + mGlobalSize;
        if(Edges & EDGE_LEFT)   min.x = glm::max(min.x, frame.SafeInsets.x);
        if(Edges & EDGE_TOP)    min.y = glm::max(min.y, frame.SafeInsets.y);
        if(Edges & EDGE_RIGHT)  max.x = glm::min(max.x, frame.ScreenSize.x - frame.SafeInsets.z);
        if(Edges & EDGE_BOTTOM) max.y = glm::min(max.y, frame.ScreenSize.y - frame.SafeInsets.w);

        const glm::vec2 size = glm::max(max - min, glm::vec2(0.0f));
        for(auto& child : mChildren)
            child->UpdateLayout(min, size, mLayoutScale);
    }

    // -----------------------------------------------------------------------------------------------
    // LayoutGroup
    // -----------------------------------------------------------------------------------------------

    int UI::LayoutGroup::GetColumnCount(float innerWidth) const
    {
        if(Columns > 0)
            return Columns;
        const float cell = CellSize.x + Spacing.x;
        return cell > 0.0f ? glm::max(static_cast<int>((innerWidth + Spacing.x) / cell), 1) : 1;
    }

    glm::vec2 UI::LayoutGroup::ComputeContentSize(float innerWidth) const
    {
        int count = 0;
        glm::vec2 sum = { 0.0f, 0.0f };
        glm::vec2 largest = { 0.0f, 0.0f };
        for(const auto& child : mChildren)
        {
            if(!child->IsVisible())
                continue;
            const glm::vec2 size = child->GetPreferredSize();
            sum += size;
            largest = glm::max(largest, size);
            count++;
        }

        glm::vec2 content = { 0.0f, 0.0f };
        if(count > 0)
        {
            const float gaps = static_cast<float>(count - 1);
            switch(Direction)
            {
                case LayoutDirection::VERTICAL:   content = { largest.x, sum.y + Spacing.y * gaps }; break;
                case LayoutDirection::HORIZONTAL: content = { sum.x + Spacing.x * gaps, largest.y }; break;
                case LayoutDirection::GRID:
                {
                    const int columns = glm::min(GetColumnCount(innerWidth), count);
                    const int rows = (count + columns - 1) / columns;
                    content = { columns * CellSize.x + (columns - 1) * Spacing.x, rows * CellSize.y + (rows - 1) * Spacing.y };
                    break;
                }
            }
        }
        return content + glm::vec2(Padding.x + Padding.z, Padding.y + Padding.w);
    }

    glm::vec2 UI::LayoutGroup::GetPreferredSize() const
    {
        if(!FitContent)
            return mSize;

        const glm::vec2 content = ComputeContentSize(mSize.x - Padding.x - Padding.z);
        glm::vec2 size = mSize;
        if(Direction != LayoutDirection::GRID)
            size.x = content.x;
        size.y = content.y;
        return size;
    }

    void UI::LayoutGroup::UpdateLayout(const glm::vec2& parentPos, const glm::vec2& parentSize, float scale)
    {
        ComputeRect(parentPos, parentSize, scale);

        if(FitContent)
        {
            // Width first (a grid needs it for its column count), then resize the non stretched axes to the content and place again
            const glm::vec2 content = ComputeContentSize(mGlobalSize.x / scale - Padding.x - Padding.z);
            const glm::vec2 authored = mSize;
            if(mAnchorMin.x == mAnchorMax.x && Direction != LayoutDirection::GRID)
                mSize.x = content.x;
            if(mAnchorMin.y == mAnchorMax.y)
                mSize.y = content.y;
            ComputeRect(parentPos, parentSize, scale);
            mSize = authored; // Keep the authored size, the fit is recomputed every layout
        }

        LayoutChildren();
    }

    void UI::LayoutGroup::LayoutChildren()
    {
        const float scale = mLayoutScale;
        const glm::vec2 innerPos = mGlobalPos + glm::vec2(Padding.x, Padding.y) * scale;
        const glm::vec2 innerSize = glm::max(mGlobalSize - glm::vec2(Padding.x + Padding.z, Padding.y + Padding.w) * scale, glm::vec2(0.0f));
        const glm::vec2 spacing = Spacing * scale;

        int count = 0;
        glm::vec2 sum = { 0.0f, 0.0f };
        for(const auto& child : mChildren)
        {
            if(child->IsVisible())
            {
                sum += child->GetPreferredSize() * scale;
                count++;
            }
        }
        if(count == 0)
            return;

        const float gaps = static_cast<float>(count - 1);
        switch(Direction)
        {
            case LayoutDirection::VERTICAL:
            {
                const float sharedHeight = (innerSize.y - spacing.y * gaps) / static_cast<float>(count);
                const float totalHeight = ControlChildHeight ? innerSize.y : sum.y + spacing.y * gaps;
                float y = innerPos.y + glm::max(innerSize.y - totalHeight, 0.0f) * ChildAlignment.y;
                for(auto& child : mChildren)
                {
                    if(!child->IsVisible())
                        continue;
                    const glm::vec2 preferred = child->GetPreferredSize() * scale;
                    const float width = ControlChildWidth ? innerSize.x : preferred.x;
                    const float height = ControlChildHeight ? glm::max(sharedHeight, 0.0f) : preferred.y;
                    const float x = innerPos.x + (innerSize.x - width) * ChildAlignment.x;
                    child->LayoutAt({ x, y }, { width, height }, scale);
                    y += height + spacing.y;
                }
                break;
            }

            case LayoutDirection::HORIZONTAL:
            {
                const float sharedWidth = (innerSize.x - spacing.x * gaps) / static_cast<float>(count);
                const float totalWidth = ControlChildWidth ? innerSize.x : sum.x + spacing.x * gaps;
                float x = innerPos.x + glm::max(innerSize.x - totalWidth, 0.0f) * ChildAlignment.x;
                for(auto& child : mChildren)
                {
                    if(!child->IsVisible())
                        continue;
                    const glm::vec2 preferred = child->GetPreferredSize() * scale;
                    const float width = ControlChildWidth ? glm::max(sharedWidth, 0.0f) : preferred.x;
                    const float height = ControlChildHeight ? innerSize.y : preferred.y;
                    const float y = innerPos.y + (innerSize.y - height) * ChildAlignment.y;
                    child->LayoutAt({ x, y }, { width, height }, scale);
                    x += width + spacing.x;
                }
                break;
            }

            case LayoutDirection::GRID:
            {
                const int columns = glm::min(GetColumnCount(innerSize.x / scale), count);
                const int rows = (count + columns - 1) / columns;
                const glm::vec2 cell = CellSize * scale;
                const glm::vec2 gridSize = { columns * cell.x + (columns - 1) * spacing.x, rows * cell.y + (rows - 1) * spacing.y };
                const glm::vec2 start = innerPos + glm::max(innerSize - gridSize, glm::vec2(0.0f)) * ChildAlignment;

                int index = 0;
                for(auto& child : mChildren)
                {
                    if(!child->IsVisible())
                        continue;
                    const int column = index % columns;
                    const int row = index / columns;
                    child->LayoutAt(start + glm::vec2(column * (cell.x + spacing.x), row * (cell.y + spacing.y)), cell, scale);
                    index++;
                }
                break;
            }
        }
    }

    // -----------------------------------------------------------------------------------------------
    // TabView
    // -----------------------------------------------------------------------------------------------

    UI::TabView::TabView()
    {
        mIsInteractable = true;
        mSize = { 800.0f, 600.0f };
        ActiveTabStyle.CornerRadius = 14.0f;
    }

    void UI::TabView::GetBarRect(glm::vec2& outPos, glm::vec2& outSize) const
    {
        const float height = glm::min(TabBarHeight * mLayoutScale, mGlobalSize.y);
        outPos = { mGlobalPos.x, TabsAtBottom ? mGlobalPos.y + mGlobalSize.y - height : mGlobalPos.y };
        outSize = { mGlobalSize.x, height };
    }

    int UI::TabView::TabAt(const glm::vec2& pos) const
    {
        glm::vec2 barPos, barSize;
        GetBarRect(barPos, barSize);
        if(mChildren.empty() || pos.x < barPos.x || pos.x > barPos.x + barSize.x || pos.y < barPos.y || pos.y > barPos.y + barSize.y)
            return -1;

        const int count = static_cast<int>(mChildren.size());
        const int index = static_cast<int>((pos.x - barPos.x) / (barSize.x / static_cast<float>(count)));
        return glm::clamp(index, 0, count - 1);
    }

    void UI::TabView::SetActiveTab(int index, bool notify)
    {
        if(!mChildren.empty())
            index = glm::clamp(index, 0, static_cast<int>(mChildren.size()) - 1);
        if(index == mActiveTab)
            return;

        mActiveTab = index;
        if(notify)
            NotifyValueChanged();
    }

    void UI::TabView::LayoutChildren()
    {
        glm::vec2 barPos, barSize;
        GetBarRect(barPos, barSize);
        const glm::vec2 pagePos = { mGlobalPos.x, TabsAtBottom ? mGlobalPos.y : mGlobalPos.y + barSize.y };
        const glm::vec2 pageSize = { mGlobalSize.x, glm::max(mGlobalSize.y - barSize.y, 0.0f) };

        // Pages are laid out relative to the area under (or above) the bar, only the active one
        if(mActiveTab >= 0 && mActiveTab < static_cast<int>(mChildren.size()))
            mChildren[mActiveTab]->UpdateLayout(pagePos, pageSize, mLayoutScale);
    }

    void UI::TabView::GenerateDrawCommands(DrawList& drawList)
    {
        glm::vec2 barPos, barSize;
        GetBarRect(barPos, barSize);
        DrawImage(drawList, Bar, BarColor * mColor, barPos, barSize, mLayoutScale);

        const int count = static_cast<int>(mChildren.size());
        if(count == 0)
            return;

        const float tabWidth = barSize.x / static_cast<float>(count);
        const float inset = glm::min(6.0f * mLayoutScale, barSize.y * 0.2f);
        const float fontSize = FontSize * mLayoutScale;
        for(int i = 0; i < count; i++)
        {
            const glm::vec2 cellPos = { barPos.x + tabWidth * static_cast<float>(i), barPos.y };
            const glm::vec2 cellSize = { tabWidth, barSize.y };

            if(i == mActiveTab)
                DrawImage(drawList, ActiveTabStyle, ActiveTabColor * mColor, cellPos + inset, cellSize - inset * 2.0f, mLayoutScale);
            else if(mIsPressed && i == mPressedTab)
                DrawRect(drawList, cellPos + inset, cellPos + cellSize - inset, ActiveTabColor * glm::vec4(1.0f, 1.0f, 1.0f, 0.25f * mColor.a), ActiveTabStyle.CornerRadius * mLayoutScale);

            if(TextFont && fontSize > 0.0f)
            {
                const glm::vec2 center = cellPos + cellSize * 0.5f;
                TextSubmitCmd cmd {};
                cmd.Transform = glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(center.x, center.y, 0.0f)), glm::vec3(fontSize, fontSize, 1.0f));
                cmd.Text = mChildren[i]->GetName();
                cmd.Color = (i == mActiveTab ? ActiveTextColor : TextColor) * mColor;
                cmd.FontAsset = TextFont.Raw();
                cmd.Alignment = TextAlignment::CENTER;
                cmd.VerticalAlignment = TextVerticalAlignment::CENTER;
                drawList.AddText(cmd);
            }
        }

        if(mActiveTab >= 0 && mActiveTab < count && mChildren[mActiveTab]->IsVisible())
            mChildren[mActiveTab]->GenerateDrawCommands(drawList);
    }

    UI::Widget* UI::TabView::HitTest(float x, float y)
    {
        if(!mVisible)
            return nullptr;

        if(TabAt({ x, y }) >= 0)
            return this;

        if(mActiveTab >= 0 && mActiveTab < static_cast<int>(mChildren.size()))
            return mChildren[mActiveTab]->HitTest(x, y);
        return nullptr;
    }

    void UI::TabView::OnPointerDown(const glm::vec2& pos)
    {
        mIsPressed = true;
        mPressedTab = TabAt(pos);
    }

    void UI::TabView::OnPointerUp(const glm::vec2& pos)
    {
        if(mIsPressed && mPressedTab >= 0 && TabAt(pos) == mPressedTab)
            SetActiveTab(mPressedTab);
        mPressedTab = -1;
        Widget::OnPointerUp(pos);
    }
}
