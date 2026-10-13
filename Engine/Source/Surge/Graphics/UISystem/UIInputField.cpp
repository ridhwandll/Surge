// Copyright (c) - SurgeTechnologies - All rights reserved
#include "UIInputField.hpp"
#include "Surge/Core/Core.hpp"
#include "Surge/Graphics/Renderer/Renderer.hpp"
#include "Surge/Utility/Platform.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <cctype>

namespace Surge::UI
{
    namespace
    {
        constexpr float BASELINE_IN_LINE = 0.75f;  // Baseline from the line top, in line heights: matches UIOverlayPass's CENTER (center + 0.25 em)
        constexpr float DOUBLE_TAP_SECONDS = 0.35f;

        bool IsContinuation(char c) { return (static_cast<unsigned char>(c) & 0xC0) == 0x80; }

        int NextCodepoint(const String& text, int i)
        {
            const int size = static_cast<int>(text.size());
            if(i >= size)
                return size;
            i++;
            while(i < size && IsContinuation(text[i]))
                i++;
            return i;
        }

        int PrevCodepoint(const String& text, int i)
        {
            if(i <= 0)
                return 0;
            i--;
            while(i > 0 && IsContinuation(text[i]))
                i--;
            return i;
        }

        int CountCodepoints(std::string_view text)
        {
            int count = 0;
            for(const char c : text)
            {
                if(!IsContinuation(c))
                    count++;
            }
            return count;
        }

        void AppendUtf8(String& out, Uint codepoint)
        {
            if(codepoint < 0x80)
                out += static_cast<char>(codepoint);
            else if(codepoint < 0x800)
            {
                out += static_cast<char>(0xC0 | (codepoint >> 6));
                out += static_cast<char>(0x80 | (codepoint & 0x3F));
            }
            else if(codepoint < 0x10000)
            {
                out += static_cast<char>(0xE0 | (codepoint >> 12));
                out += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
                out += static_cast<char>(0x80 | (codepoint & 0x3F));
            }
        }

        bool IsWordChar(char c)
        {
            const unsigned char u = static_cast<unsigned char>(c);
            return u >= 0x80 || std::isalnum(u) || c == '_';
        }

        Platform::SoftKeyboardType ToKeyboardType(InputContentType type)
        {
            switch(type)
            {
                case InputContentType::INTEGER:  return Platform::SoftKeyboardType::NUMBER;
                case InputContentType::DECIMAL:  return Platform::SoftKeyboardType::DECIMAL;
                case InputContentType::EMAIL:    return Platform::SoftKeyboardType::EMAIL;
                case InputContentType::PHONE:    return Platform::SoftKeyboardType::PHONE;
                case InputContentType::PASSWORD: return Platform::SoftKeyboardType::PASSWORD;
                case InputContentType::STANDARD: break;
            }
            return Platform::SoftKeyboardType::TEXT;
        }

        Manager& GetManager()
        {
            return Core::GetRenderer()->GetUIManager();
        }
    }

    InputField::InputField(Ref<Font> font)
        : TextFont(font)
    {
        mIsInteractable = true;
        mSize = { 500.0f, 80.0f };
        Background.CornerRadius = 12.0f;
        Background.BorderWidth = 2.0f;
        Background.BorderColor = { 1.0f, 1.0f, 1.0f, 0.15f };
    }

    void InputField::Destroy()
    {
        mOnSubmit = nullptr;
        mOnFocus = nullptr;
        mOnBlur = nullptr;
        Widget::Destroy();
    }

    // -----------------------------------------------------------------------------------------------
    // Focus
    // -----------------------------------------------------------------------------------------------

    void InputField::Focus()
    {
        if(!mFocused)
            GetManager().SetFocus(this);
    }

    void InputField::Blur()
    {
        if(mFocused)
            GetManager().SetFocus(nullptr);
    }

    void InputField::OnFocus()
    {
        // A tap places the caret where it landed, focusing from code puts it at the end
        mCaret = mAnchor = mIsPressed ? OffsetAtPoint(mPressPos) : static_cast<int>(mText.size());
        mFocused = true;
        mLastEditTime = GetFrameInfo().Time;
        Platform::ShowSoftKeyboard(ToKeyboardType(ContentType), MultiLine, { mText, SelectionMin(), SelectionMax() });
        if(mOnFocus)
            mOnFocus->Invoke();
    }

    void InputField::OnBlur()
    {
        mFocused = false;
        mSelecting = false;
        mAnchor = mCaret;
        mScroll = { 0.0f, 0.0f };
        Platform::HideSoftKeyboard();
        if(mOnBlur)
            mOnBlur->Invoke();
    }

    void InputField::Submit()
    {
        Ref<IEventCallback> callback = mOnSubmit; // Blur callbacks may replace it
        if(!MultiLine)
            Blur();
        if(callback)
            callback->Invoke();
    }

    // -----------------------------------------------------------------------------------------------
    // Editing
    // -----------------------------------------------------------------------------------------------

    String InputField::Filter(std::string_view text, bool allowLeadingMinus, bool hasDecimalPoint) const
    {
        String out;
        out.reserve(text.size());
        for(const char c : text)
        {
            const unsigned char u = static_cast<unsigned char>(c);
            if(c == '\r')
                continue;
            if(c == '\n')
            {
                if(MultiLine && ContentType == InputContentType::STANDARD)
                    out += '\n';
                continue;
            }
            if(u < 32 || u == 127)
                continue; // Control characters (Ctrl+letter on Windows, tabs)

            switch(ContentType)
            {
                case InputContentType::INTEGER:
                case InputContentType::DECIMAL:
                    if(std::isdigit(u))
                        out += c;
                    else if(c == '-' && allowLeadingMinus && out.empty())
                        out += c;
                    else if((c == '.' || c == ',') && ContentType == InputContentType::DECIMAL && !hasDecimalPoint)
                    {
                        out += '.';
                        hasDecimalPoint = true;
                    }
                    break;
                case InputContentType::PHONE:
                    if(std::isdigit(u) || c == '+' || c == '-' || c == '(' || c == ')' || c == '*' || c == '#' || c == ' ')
                        out += c;
                    break;
                case InputContentType::EMAIL:
                    if(c != ' ')
                        out += c;
                    break;
                case InputContentType::STANDARD:
                case InputContentType::PASSWORD:
                    out += c;
                    break;
            }
        }
        return out;
    }

    String InputField::ClampLength(std::string_view text, int availableCodepoints) const
    {
        if(MaxLength <= 0)
            return String(text);
        if(availableCodepoints <= 0)
            return String();

        int count = 0;
        size_t end = 0;
        while(end < text.size())
        {
            size_t next = end + 1;
            while(next < text.size() && IsContinuation(text[next]))
                next++;
            if(++count > availableCodepoints)
                break;
            end = next;
        }
        return String(text.substr(0, end));
    }

    void InputField::ReplaceSelection(std::string_view insert)
    {
        const int from = SelectionMin();
        const int to = SelectionMax();

        // Number rules look at the text that stays around the insertion
        const bool allowMinus = from == 0 && !(to < static_cast<int>(mText.size()) && mText[to] == '-');
        const bool hasPoint = mText.find('.') < static_cast<size_t>(from) || mText.find('.', to) != String::npos;
        String filtered = Filter(insert, allowMinus, hasPoint);

        if(MaxLength > 0)
        {
            const int kept = CountCodepoints(mText) - CountCodepoints(std::string_view(mText).substr(from, to - from));
            filtered = ClampLength(filtered, MaxLength - kept);
        }

        if(filtered.empty() && from == to)
            return;

        mText.replace(from, to - from, filtered);
        mCaret = mAnchor = from + static_cast<int>(filtered.size());
        OnEdited();
    }

    void InputField::DeleteSelectionOrRange(int from, int to)
    {
        if(HasSelection())
        {
            ReplaceSelection({});
            return;
        }

        const int lo = glm::clamp(glm::min(from, to), 0, static_cast<int>(mText.size()));
        const int hi = glm::clamp(glm::max(from, to), 0, static_cast<int>(mText.size()));
        if(lo == hi)
            return;

        mText.erase(lo, hi - lo);
        mCaret = mAnchor = lo;
        OnEdited();
    }

    void InputField::MoveCaret(int position, bool extendSelection)
    {
        mCaret = glm::clamp(position, 0, static_cast<int>(mText.size()));
        if(!extendSelection)
            mAnchor = mCaret;
        mLastEditTime = GetFrameInfo().Time;
        PushStateToPlatform();
    }

    void InputField::OnEdited()
    {
        mDisplayDirty = true;
        mLastEditTime = GetFrameInfo().Time;
        NotifyValueChanged();
        PushStateToPlatform();
    }

    void InputField::PushStateToPlatform() const
    {
        if(mFocused)
            Platform::SetTextInputState({ mText, SelectionMin(), SelectionMax() });
    }

    int InputField::WordBoundary(int from, int direction) const
    {
        const int size = static_cast<int>(mText.size());
        int i = glm::clamp(from, 0, size);
        if(direction < 0)
        {
            while(i > 0 && !IsWordChar(mText[i - 1])) i--;
            while(i > 0 && IsWordChar(mText[i - 1])) i--;
        }
        else
        {
            while(i < size && IsWordChar(mText[i])) i++;
            while(i < size && !IsWordChar(mText[i])) i++;
        }
        return i;
    }

    void InputField::SelectWordAt(int position)
    {
        const int size = static_cast<int>(mText.size());
        int begin = glm::clamp(position, 0, size);
        int end = begin;
        while(begin > 0 && IsWordChar(mText[begin - 1])) begin--;
        while(end < size && IsWordChar(mText[end])) end++;
        mAnchor = begin;
        mCaret = end;
        mLastEditTime = GetFrameInfo().Time;
        PushStateToPlatform();
    }

    void InputField::SetText(const String& text, bool notify)
    {
        String filtered = ClampLength(Filter(text, true, false), MaxLength);
        if(filtered == mText)
            return;

        mText = std::move(filtered);
        mCaret = mAnchor = static_cast<int>(mText.size());
        mDisplayDirty = true;
        if(notify)
            NotifyValueChanged();
        PushStateToPlatform();
    }

    void InputField::ApplyPlatformTextState(const String& text, int selectionStart, int selectionEnd)
    {
        // The IME may produce anything (pasted text, line breaks in a single line field): filter it like typed text
        String filtered = ClampLength(Filter(text, true, false), MaxLength);
        const bool corrected = filtered != text;
        const bool changed = filtered != mText;
        mText = std::move(filtered);

        const int size = static_cast<int>(mText.size());
        auto toBoundary = [&](int offset) {
            offset = glm::clamp(offset, 0, size);
            while(offset > 0 && offset < size && IsContinuation(mText[offset]))
                offset--;
            return offset;
        };
        mAnchor = corrected ? size : toBoundary(selectionStart);
        mCaret = corrected ? size : toBoundary(selectionEnd);

        mDisplayDirty = true;
        mLastEditTime = GetFrameInfo().Time;
        if(changed)
            NotifyValueChanged();
        if(corrected)
            PushStateToPlatform(); // Give the IME the filtered text back
    }

    bool InputField::OnKeyPressed(Key key, bool ctrl, bool shift)
    {
        if(!mFocused)
            return false;

        const int size = static_cast<int>(mText.size());
        switch(key)
        {
            case Key::Left:
                if(HasSelection() && !shift)
                    MoveCaret(SelectionMin(), false);
                else
                    MoveCaret(ctrl ? WordBoundary(mCaret, -1) : PrevCodepoint(mText, mCaret), shift);
                return true;

            case Key::Right:
                if(HasSelection() && !shift)
                    MoveCaret(SelectionMax(), false);
                else
                    MoveCaret(ctrl ? WordBoundary(mCaret, 1) : NextCodepoint(mText, mCaret), shift);
                return true;

            case Key::Up:
            case Key::Down:
            {
                UpdateDisplay();
                const int line = LineOfDisplayOffset(ToDisplay(mCaret));
                const int target = line + (key == Key::Up ? -1 : 1);
                if(!MultiLine || target < 0 || target >= static_cast<int>(mLines.size()))
                    MoveCaret(key == Key::Up ? 0 : size, shift);
                else
                    MoveCaret(OffsetInLine(target, DisplayX(line, ToDisplay(mCaret))), shift);
                return true;
            }

            case Key::Home:
            case Key::End:
            {
                if(ctrl || !MultiLine)
                {
                    MoveCaret(key == Key::Home ? 0 : size, shift);
                    return true;
                }
                UpdateDisplay();
                const Line& line = mLines[LineOfDisplayOffset(ToDisplay(mCaret))];
                MoveCaret(FromDisplay(key == Key::Home ? line.Begin : line.End), shift);
                return true;
            }

            case Key::Backspace:
                DeleteSelectionOrRange(ctrl ? WordBoundary(mCaret, -1) : PrevCodepoint(mText, mCaret), mCaret);
                return true;

            case Key::Delete:
                DeleteSelectionOrRange(mCaret, ctrl ? WordBoundary(mCaret, 1) : NextCodepoint(mText, mCaret));
                return true;

            case Key::Enter:
                if(MultiLine && !ctrl)
                    ReplaceSelection("\n");
                else
                    Submit();
                return true;

            case Key::A:
                if(!ctrl)
                    return false;
                mAnchor = 0;
                MoveCaret(size, true);
                return true;

            case Key::C:
            case Key::X:
                if(!ctrl)
                    return false;
                if(HasSelection() && ContentType != InputContentType::PASSWORD)
                {
                    Platform::SetClipboardText(mText.substr(SelectionMin(), SelectionMax() - SelectionMin()));
                    if(key == Key::X)
                        ReplaceSelection({});
                }
                return true;

            case Key::V:
                if(!ctrl)
                    return false;
                ReplaceSelection(Platform::GetClipboardText());
                return true;

            default:
                return false;
        }
    }

    void InputField::OnTextInput(Uint codepoint)
    {
        if(!mFocused || codepoint < 32 || codepoint == 127)
            return; // Control characters come with their key events (Backspace, Enter, Ctrl+...)

        String utf8;
        AppendUtf8(utf8, codepoint);
        ReplaceSelection(utf8);
    }

    // -----------------------------------------------------------------------------------------------
    // Pointer
    // -----------------------------------------------------------------------------------------------

    void InputField::OnPointerDown(const glm::vec2& pos)
    {
        mIsPressed = true;
        mPressPos = pos;
        if(!mFocused)
            return; // Focus happens on release (a drag may still turn into scrolling)

        const float now = GetFrameInfo().Time;
        const int offset = OffsetAtPoint(pos);
        if(now - mLastTapTime < DOUBLE_TAP_SECONDS && glm::length(pos - mLastTapPos) < 24.0f * mLayoutScale)
        {
            SelectWordAt(offset);
            mLastTapTime = -10.0f;
            mSelecting = false;
            return;
        }

        MoveCaret(offset, false);
        mLastTapTime = now;
        mLastTapPos = pos;
        mSelecting = true;
    }

    void InputField::OnPointerDrag(const glm::vec2& pos)
    {
        if(!mSelecting)
            return;

        mCaret = OffsetAtPoint(pos); // Anchor stays where the press started
        mLastEditTime = GetFrameInfo().Time;
    }

    void InputField::OnPointerUp(const glm::vec2& pos)
    {
        if(mSelecting)
        {
            mSelecting = false;
            PushStateToPlatform();
        }
        if(mIsPressed && !mFocused)
            Focus(); // mIsPressed is still set: OnFocus places the caret at the press
        Widget::OnPointerUp(pos);
    }

    // -----------------------------------------------------------------------------------------------
    // Layout
    // -----------------------------------------------------------------------------------------------

    void InputField::GetTextArea(glm::vec2& outPos, glm::vec2& outSize) const
    {
        const float padding = TextPadding * mLayoutScale;
        outPos = mGlobalPos + padding;
        outSize = glm::max(mGlobalSize - padding * 2.0f, glm::vec2(0.0f));
    }

    float InputField::GetLineHeight() const
    {
        const Font* font = TextFont.Raw();
        return (font ? font->GetLineHeight() : 1.2f) * GetFontPixels();
    }

    float InputField::GetLineTop(int line) const
    {
        glm::vec2 areaPos, areaSize;
        GetTextArea(areaPos, areaSize);
        const float lineHeight = GetLineHeight();
        if(!MultiLine)
            return areaPos.y + (areaSize.y - lineHeight) * 0.5f; // One line, centered
        return areaPos.y + static_cast<float>(line) * lineHeight - mScroll.y;
    }

    void InputField::UpdateDisplay()
    {
        const Font* font = TextFont.Raw();
        glm::vec2 areaPos, areaSize;
        GetTextArea(areaPos, areaSize);
        const float fontPixels = GetFontPixels();
        const float width = MultiLine ? areaSize.x : 0.0f;
        if(!mDisplayDirty && width == mDisplayWidth && fontPixels == mDisplayFontPixels && font == mDisplayFont && mDisplaySource == mText)
            return;

        mDisplayDirty = false;
        mDisplaySource = mText;
        mDisplayWidth = width;
        mDisplayFontPixels = fontPixels;
        mDisplayFont = font;

        if(ContentType == InputContentType::PASSWORD)
            mDisplay.assign(static_cast<size_t>(CountCodepoints(mText)), '*');
        else
            mDisplay = mText;

        mLines.clear();
        const int size = static_cast<int>(mDisplay.size());
        if(!MultiLine)
        {
            mLines.push_back({ 0, size });
            return;
        }

        // Greedy word wrap per paragraph: break after the last space that fits, or inside a word longer than the line
        const float maxWidth = fontPixels > 0.0f ? width / fontPixels : 0.0f;
        int paragraphStart = 0;
        while(true)
        {
            const size_t newline = mDisplay.find('\n', static_cast<size_t>(paragraphStart));
            const int paragraphEnd = newline == String::npos ? size : static_cast<int>(newline);

            int lineStart = paragraphStart;
            float lineWidth = 0.0f;
            Uint previous = 0;
            int lastSpace = -1;
            for(int i = lineStart; i < paragraphEnd; i++)
            {
                const char c = mDisplay[i];
                const FontGlyph* glyph = font ? font->GetGlyph(c) : nullptr;
                const float advance = glyph ? (previous != 0 ? font->GetKerning(previous, c) : 0.0f) + glyph->Advance : 0.0f;

                if(maxWidth > 0.0f && glyph && lineWidth + advance > maxWidth && i > lineStart)
                {
                    const bool atSpace = lastSpace >= lineStart;
                    mLines.push_back({ lineStart, atSpace ? lastSpace : i });
                    lineStart = atSpace ? lastSpace + 1 : i; // The space at the break is not drawn
                    i = lineStart - 1;
                    lineWidth = 0.0f;
                    previous = 0;
                    lastSpace = -1;
                    continue;
                }

                lineWidth += advance;
                if(glyph)
                    previous = c;
                if(c == ' ')
                    lastSpace = i;
            }
            mLines.push_back({ lineStart, paragraphEnd });

            if(paragraphEnd >= size)
                break;
            paragraphStart = paragraphEnd + 1;
        }
    }

    int InputField::ToDisplay(int textOffset) const
    {
        if(ContentType != InputContentType::PASSWORD)
            return textOffset;
        return CountCodepoints(std::string_view(mText).substr(0, static_cast<size_t>(glm::clamp(textOffset, 0, static_cast<int>(mText.size())))));
    }

    int InputField::FromDisplay(int displayOffset) const
    {
        if(ContentType != InputContentType::PASSWORD)
            return displayOffset;

        int offset = 0;
        for(int i = 0; i < displayOffset && offset < static_cast<int>(mText.size()); i++)
            offset = NextCodepoint(mText, offset);
        return offset;
    }

    int InputField::LineOfDisplayOffset(int displayOffset) const
    {
        // Last line starting at or before the offset: a caret on a wrap point sits at the start of the next line
        int line = 0;
        for(int i = 0; i < static_cast<int>(mLines.size()); i++)
        {
            if(mLines[i].Begin > displayOffset)
                break;
            line = i;
        }
        return line;
    }

    float InputField::DisplayX(int line, int displayOffset) const
    {
        if(line < 0 || line >= static_cast<int>(mLines.size()))
            return 0.0f;

        const Line& l = mLines[line];
        const int end = glm::clamp(displayOffset, l.Begin, l.End);
        return MeasureText(TextFont.Raw(), std::string_view(mDisplay).substr(static_cast<size_t>(l.Begin), static_cast<size_t>(end - l.Begin)));
    }

    int InputField::OffsetInLine(int line, float xEm) const
    {
        if(line < 0 || line >= static_cast<int>(mLines.size()))
            return 0;

        // Walk the glyphs, the caret goes before a glyph when x is left of its middle
        const Font* font = TextFont.Raw();
        const Line& l = mLines[line];
        float width = 0.0f;
        Uint previous = 0;
        for(int i = l.Begin; i < l.End; i = NextCodepoint(mDisplay, i))
        {
            const char c = mDisplay[i];
            const FontGlyph* glyph = font ? font->GetGlyph(c) : nullptr;
            const float advance = glyph ? (previous != 0 ? font->GetKerning(previous, c) : 0.0f) + glyph->Advance : 0.0f;
            if(xEm < width + advance * 0.5f)
                return FromDisplay(i);
            width += advance;
            if(glyph)
                previous = c;
        }
        return FromDisplay(l.End);
    }

    int InputField::OffsetAtPoint(const glm::vec2& pos)
    {
        UpdateDisplay();
        glm::vec2 areaPos, areaSize;
        GetTextArea(areaPos, areaSize);
        const float fontPixels = GetFontPixels();
        if(fontPixels <= 0.0f || mLines.empty())
            return static_cast<int>(mText.size());

        int line = 0;
        if(MultiLine)
            line = glm::clamp(static_cast<int>(std::floor((pos.y - areaPos.y + mScroll.y) / GetLineHeight())), 0, static_cast<int>(mLines.size()) - 1);
        return OffsetInLine(line, (pos.x - areaPos.x + mScroll.x) / fontPixels);
    }

    void InputField::EnsureCaretVisible()
    {
        glm::vec2 areaPos, areaSize;
        GetTextArea(areaPos, areaSize);
        const float fontPixels = GetFontPixels();
        const float lineHeight = GetLineHeight();
        const int caret = ToDisplay(mCaret);
        const int line = LineOfDisplayOffset(caret);
        const float margin = 2.0f * mLayoutScale;

        if(!MultiLine)
        {
            const float caretX = DisplayX(line, caret) * fontPixels;
            if(caretX - mScroll.x > areaSize.x - margin) mScroll.x = caretX - areaSize.x + margin;
            if(caretX - mScroll.x < 0.0f)                mScroll.x = caretX;
            const float textWidth = MeasureText(TextFont.Raw(), mDisplay) * fontPixels;
            mScroll.x = glm::clamp(mScroll.x, 0.0f, glm::max(textWidth + margin - areaSize.x, 0.0f));
            mScroll.y = 0.0f;
        }
        else
        {
            const float top = static_cast<float>(line) * lineHeight;
            if(top - mScroll.y < 0.0f)                       mScroll.y = top;
            if(top + lineHeight - mScroll.y > areaSize.y)    mScroll.y = top + lineHeight - areaSize.y;
            const float contentHeight = static_cast<float>(mLines.size()) * lineHeight;
            mScroll.y = glm::clamp(mScroll.y, 0.0f, glm::max(contentHeight - areaSize.y, 0.0f));
            mScroll.x = 0.0f;
        }
    }

    // -----------------------------------------------------------------------------------------------
    // Drawing
    // -----------------------------------------------------------------------------------------------

    void InputField::GenerateDrawCommands(DrawList& drawList)
    {
        UpdateDisplay();

        // Focus is shown on the border (temporarily swapped, the style keeps its own color)
        const glm::vec4 borderColor = Background.BorderColor;
        if(mFocused)
            Background.BorderColor = FocusedBorderColor;
        DrawImage(drawList, Background, BackgroundColor * mColor, mGlobalPos, mGlobalSize, mLayoutScale);
        Background.BorderColor = borderColor;

        const Font* font = TextFont.Raw();
        const float fontPixels = GetFontPixels();
        if(!font || fontPixels <= 0.0f)
        {
            GenerateChildDrawCommands(drawList);
            return;
        }

        if(mFocused)
            EnsureCaretVisible();

        glm::vec2 areaPos, areaSize;
        GetTextArea(areaPos, areaSize);
        const float lineHeight = GetLineHeight();
        const float caretWidth = glm::max(2.0f * mLayoutScale, 1.0f);
        const uint16_t previousClip = drawList.PushClip(areaPos - glm::vec2(caretWidth, 0.0f), areaPos + areaSize + glm::vec2(caretWidth, 0.0f));

        auto submitLine = [&](std::string_view text, float x, float lineTop, const glm::vec4& color) {
            TextSubmitCmd cmd {};
            const glm::vec3 origin = { x, lineTop + lineHeight * BASELINE_IN_LINE, 0.0f };
            cmd.Transform = glm::scale(glm::translate(glm::mat4(1.0f), origin), glm::vec3(fontPixels, fontPixels, 1.0f));
            cmd.Text = String(text);
            cmd.Color = color;
            cmd.FontAsset = font;
            cmd.Alignment = TextAlignment::LEFT;
            cmd.VerticalAlignment = TextVerticalAlignment::BASELINE;
            drawList.AddText(cmd);
        };

        const float textX = areaPos.x - mScroll.x;
        if(mText.empty())
        {
            if(!Placeholder.empty())
                submitLine(Placeholder, areaPos.x, GetLineTop(0), PlaceholderColor * mColor);
        }
        else
        {
            if(mFocused && HasSelection())
            {
                const int from = ToDisplay(SelectionMin());
                const int to = ToDisplay(SelectionMax());
                for(int i = 0; i < static_cast<int>(mLines.size()); i++)
                {
                    const int begin = glm::max(from, mLines[i].Begin);
                    const int end = glm::min(to, mLines[i].End);
                    if(begin > end || (begin == end && to <= mLines[i].End))
                        continue;

                    // A selection running past the line end (over its break) shows a small tail
                    const float x0 = textX + DisplayX(i, begin) * fontPixels;
                    float x1 = textX + DisplayX(i, end) * fontPixels;
                    if(to > mLines[i].End)
                        x1 += fontPixels * 0.3f;
                    const float top = GetLineTop(i);
                    DrawRect(drawList, { x0, top }, { x1, top + lineHeight }, SelectionColor * mColor);
                }
            }

            for(int i = 0; i < static_cast<int>(mLines.size()); i++)
            {
                const float top = GetLineTop(i);
                if(top + lineHeight < areaPos.y || top > areaPos.y + areaSize.y || mLines[i].End <= mLines[i].Begin)
                    continue; // Scrolled out or empty
                submitLine(std::string_view(mDisplay).substr(static_cast<size_t>(mLines[i].Begin), static_cast<size_t>(mLines[i].End - mLines[i].Begin)), textX, top, TextColor * mColor);
            }
        }

        // Caret, blinking at 1 Hz, solid right after an edit or a move
        if(mFocused && !HasSelection() && std::fmod(GetFrameInfo().Time - mLastEditTime, 1.0f) < 0.5f)
        {
            const int caret = ToDisplay(mCaret);
            const int line = LineOfDisplayOffset(caret);
            const float x = (mText.empty() ? areaPos.x : textX + DisplayX(line, caret) * fontPixels);
            const float top = GetLineTop(line);
            DrawRect(drawList, { x - caretWidth * 0.5f, top + lineHeight * 0.1f }, { x + caretWidth * 0.5f, top + lineHeight * 0.9f }, CaretColor * mColor);
        }

        drawList.PopClip(previousClip);
        GenerateChildDrawCommands(drawList);
    }
}
