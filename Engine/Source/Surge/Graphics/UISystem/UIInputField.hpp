// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Graphics/UISystem/UIWidgets.hpp"

namespace Surge::UI
{
    // Filters what can be typed and picks the soft keyboard layout on Android
    enum class InputContentType : uint8_t
    {
        STANDARD,
        INTEGER,  // Digits, leading '-'
        DECIMAL,  // Digits, one '.', leading '-'
        EMAIL,    // No spaces, email keyboard
        PHONE,    // Digits and + - ( ) * # space, phone keyboard
        PASSWORD, // Drawn as '*', never copied
    };

    /*
    * Text entry. Tap to focus (opens the soft keyboard on Android), tap/drag to place the caret and select, double tap selects a word
    * Keys: arrows, Home/End, Backspace/Delete (Ctrl = word), Ctrl+A/C/X/V, Enter submits (inserts a line break when MultiLine), Tab = next field, Escape = blur
    * Text is UTF-8; caret/selection are byte offsets kept on code point boundaries. Glyphs the font doesn't have are not drawn (Latin fonts)
    * MultiLine wraps at the field width and scrolls vertically to keep the caret visible, a single line scrolls horizontally
    */
    class InputField final : public Widget
    {
    public:
        InputField(Ref<Font> font);

        virtual WidgetType GetType() const override { return WidgetType::INPUT_FIELD; }
        virtual void GenerateDrawCommands(DrawList& drawList) override;
        virtual void Destroy() override;

        virtual void OnPointerDown(const glm::vec2& pos) override;
        virtual void OnPointerDrag(const glm::vec2& pos) override;
        virtual void OnPointerUp(const glm::vec2& pos) override;
        virtual void CancelPress() override { mIsPressed = false; mSelecting = false; }
        virtual bool WantsDrag() const override { return mFocused; } // Unfocused fields let a scroll view take the drag

        virtual bool IsFocusable() const override { return true; }
        virtual void OnFocus() override;
        virtual void OnBlur() override;
        virtual bool OnKeyPressed(Key key, bool ctrl, bool shift) override;
        virtual void OnTextInput(Uint codepoint) override;

        void SetText(const String& text, bool notify = false);
        const String& GetText() const { return mText; }
        bool IsFocused() const { return mFocused; }
        void Focus();
        void Blur();
        void Submit(); // Enter / IME "Done": blurs a single line field, then fires OnSubmit

        void SetOnSubmit(Ref<IEventCallback> callback) { mOnSubmit = callback; }
        void SetOnFocus(Ref<IEventCallback> callback) { mOnFocus = callback; }
        void SetOnBlur(Ref<IEventCallback> callback) { mOnBlur = callback; }

        // Text + selection edited by the platform IME (Android soft keyboard), byte offsets
        void ApplyPlatformTextState(const String& text, int selectionStart, int selectionEnd);

    public:
        String Placeholder;
        InputContentType ContentType = InputContentType::STANDARD;
        bool MultiLine = false;
        int MaxLength = 0; // Code points, 0 = unlimited
        Ref<Font> TextFont;
        float FontSize = 32.0f;
        float TextPadding = 16.0f; // UI units
        ImageStyle Background;
        glm::vec4 BackgroundColor = { 0.1f, 0.11f, 0.13f, 1.0f };
        glm::vec4 FocusedBorderColor = { 0.25f, 0.6f, 1.0f, 1.0f };
        glm::vec4 TextColor = { 1.0f, 1.0f, 1.0f, 1.0f };
        glm::vec4 PlaceholderColor = { 1.0f, 1.0f, 1.0f, 0.35f };
        glm::vec4 CaretColor = { 1.0f, 1.0f, 1.0f, 1.0f };
        glm::vec4 SelectionColor = { 0.25f, 0.6f, 1.0f, 0.45f };

    private:
        struct Line
        {
            int Begin; // Display string byte offsets, End excludes the break character
            int End;
        };

        // Editing
        void ReplaceSelection(std::string_view insert);
        void DeleteSelectionOrRange(int from, int to);
        void MoveCaret(int position, bool extendSelection);
        void OnEdited();
        void PushStateToPlatform() const;
        String Filter(std::string_view text, bool allowLeadingMinus, bool hasDecimalPoint) const; // Drops what the content type doesn't accept
        String ClampLength(std::string_view text, int availableCodepoints) const;
        bool HasSelection() const { return mCaret != mAnchor; }
        int SelectionMin() const { return glm::min(mCaret, mAnchor); }
        int SelectionMax() const { return glm::max(mCaret, mAnchor); }
        int WordBoundary(int from, int direction) const;
        void SelectWordAt(int position);

        // Layout (display string = mText, or one '*' per code point for passwords)
        void UpdateDisplay();
        int ToDisplay(int textOffset) const;
        int FromDisplay(int displayOffset) const;
        int LineOfDisplayOffset(int displayOffset) const;
        float DisplayX(int line, int displayOffset) const; // Em units from the line start
        int OffsetInLine(int line, float xEm) const;       // Text offset closest to x on a line
        int OffsetAtPoint(const glm::vec2& pos);            // Text offset under a UI pixel position
        void GetTextArea(glm::vec2& outPos, glm::vec2& outSize) const;
        float GetLineHeight() const;                         // Pixels
        float GetLineTop(int line) const;                    // Pixels, scroll applied
        float GetFontPixels() const { return FontSize * mLayoutScale; }
        void EnsureCaretVisible();

    private:
        String mText;
        int mCaret = 0;  // Byte offsets into mText
        int mAnchor = 0;
        bool mFocused = false;
        bool mSelecting = false;
        glm::vec2 mPressPos = { 0.0f, 0.0f };
        float mLastTapTime = -10.0f;
        glm::vec2 mLastTapPos = { 0.0f, 0.0f };
        float mLastEditTime = 0.0f; // Caret blink restarts on edits
        glm::vec2 mScroll = { 0.0f, 0.0f }; // Pixels

        // Display cache, rebuilt when the text, font, size or width changes
        String mDisplay;
        Vector<Line> mLines;
        String mDisplaySource;
        float mDisplayWidth = -1.0f;
        float mDisplayFontPixels = -1.0f;
        const Font* mDisplayFont = nullptr;
        bool mDisplayDirty = true;

        Ref<IEventCallback> mOnSubmit;
        Ref<IEventCallback> mOnFocus;
        Ref<IEventCallback> mOnBlur;
    };

} // namespace Surge::UI
