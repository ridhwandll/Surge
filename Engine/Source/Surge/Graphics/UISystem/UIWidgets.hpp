// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Memory.hpp"
#include "Surge/Core/Defines.hpp"
#include "Surge/Core/Vector.hpp"
#include "Surge/Core/String.hpp"
#include "Surge/Core/Profiler.hpp"
#include "Surge/Core/Input/KeyCodes.hpp"
#include "Surge/Graphics/RenderGraph/FrameBlackboard.hpp"

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <string_view>

namespace Surge::UI
{
    class IEventCallback : public RefCounted
    {
    public:
        virtual ~IEventCallback() = default;
        virtual void Invoke() = 0;
    };

    /*
    * (Rid) Example Usage of UI::IEventCallback
    *
    *   // LuaEventCallback is used in Surge/ScriptEngine/UIBindings.cpp to bind Lua functions to UI events.
    *      If you are in C++ land, you can just use std::function<void()> instead of sol::protected_function
    *   class LuaEventCallback : public UI::IEventCallback
    *   {
    *   public:
    *       LuaEventCallback(sol::protected_function func) // OR std::function&& of you are in C++ Land
    *           : mFunc(std::move(func)) {}
    *
    *       void Invoke() override
    *       {
    *           // if (mFunc) mFunc(); // If you are in C++ land, you can just call the function directly
    *
    *           if(mFunc.valid())
    *           {
    *               auto result = mFunc();
    *               if(!result.valid())
    *               {
    *                   sol::error err = result;
    *                   Log<Severity::Error>("Lua UI Callback Error: {}", err.what());
    *               }
    *           }
    *       }
    *   private:
    *       sol::protected_function mFunc; // OR std::function<void()> of you are in C++ Land
    *   };
    */

    enum class WidgetType
    {
        BASE_WIDGET, // Empty container
        IMAGE,
        TEXT,
        BUTTON,
        IMAGE_BUTTON,
        PROGRESS_BAR,
        TOGGLE,
        SLIDER,
        JOYSTICK,
        SCROLL_VIEW,
        LAYOUT_GROUP,
        INPUT_FIELD,
        DROPDOWN,
        MODAL,
        TAB_VIEW,
        LIST_VIEW,
        SPINNER,
        SAFE_AREA,
    };

    enum class ImageMode : uint8_t
    {
        SIMPLE, // Texture stretched over the rect
        SLICED, // 9-slice: corners keep their size, edges/center stretch (SliceBorder)
        FILLED, // Only FillAmount of the rect is drawn, the texture is cropped through its UVs instead of squashed
    };

    enum class FillOrigin : uint8_t { LEFT, RIGHT, TOP, BOTTOM }; // Edge the fill grows from (also the direction of sliders/progress bars)
    enum class TextOverflow : uint8_t { WRAP, TRUNCATE, NO_WRAP }; // Wrap at the rect width, cut the line with "...", or let it spill
    enum class ToggleStyle : uint8_t { CHECKBOX, SWITCH };
    enum class JoystickMode : uint8_t { FIXED, FLOATING }; // FLOATING: the stick centers where the finger lands inside the rect
    enum class LayoutDirection : uint8_t { HORIZONTAL, VERTICAL, GRID };

    // Per frame values every widget can read, written by UI::Manager before layout
    struct FrameInfo
    {
        float Time = 0.0f;      // Seconds, unscaled (UI keeps animating while the game is paused)
        float DeltaTime = 0.0f;
        glm::vec2 ScreenSize = { 0.0f, 0.0f };
        glm::vec4 SafeInsets = { 0.0f, 0.0f, 0.0f, 0.0f }; // Pixels (left, top, right, bottom): notch, status and navigation bars
    };
    FrameInfo& GetFrameInfo();

    // Where a widget hierarchy writes its draw commands, Order keeps sprites and text interleaved in hierarchy order
    struct DrawList
    {
        Vector<UISpriteCmd>& Sprites;
        Vector<TextSubmitCmd>& Texts;
        Vector<UIDrawItem>& Order;
        Vector<glm::vec4>& ClipRects;
        uint16_t CurrentClipIndex = UI_NO_CLIP; // Current clip rect, every command added inherits it

        void AddSprite(const UISpriteCmd& cmd);
        void AddText(const TextSubmitCmd& cmd);

        // Clips everything added until PopClip to the rect (intersected with the current clip). Returns the clip to restore
        uint16_t PushClip(const glm::vec2& min, const glm::vec2& max);
        void PopClip(uint16_t previous) { CurrentClipIndex = previous; }
        bool IsClippedOut(const glm::vec2& min, const glm::vec2& max) const;
    };

    // How one rectangular part of a widget is drawn (an Image, a button background, a slider handle...)
    struct ImageStyle
    {
        ImageHandle Texture = ImageHandle::Invalid(); // Invalid = plain color (white texture)
        Ref<Asset> TextureAsset;                      // Keeps the owning Texture2D alive for as long as the RHI image is referenced
        glm::vec2 TextureSize = { 0.0f, 0.0f };       // Pixels, cached by SetTexture (9-slice, preserve aspect)

        ImageMode Mode = ImageMode::SIMPLE;
        glm::vec4 SliceBorder = { 0.0f, 0.0f, 0.0f, 0.0f }; // SLICED: left, top, right, bottom in texture pixels
        float SliceScale = 1.0f;                            // SLICED: border size multiplier (UI units per texture pixel)
        bool FillCenter = true;                             // SLICED: false draws only the frame
        float FillAmount = 1.0f;                            // FILLED: 0..1
        FillOrigin Origin = FillOrigin::LEFT;               // FILLED
        bool PreserveAspect = false;                        // Fit the texture inside the rect without stretching it

        float CornerRadius = 0.0f; // UI units, procedural rounded corners (no texture needed), ignored by SLICED
        float BorderWidth = 0.0f;  // UI units, drawn as a second rounded rect behind the part
        glm::vec4 BorderColor = { 0.0f, 0.0f, 0.0f, 1.0f };

        void SetTexture(ImageHandle texture, Ref<Asset> textureAsset);
    };

    // Draws a part, pos/size in pixels, scale = canvas scale (UI units -> pixels)
    void DrawImage(DrawList& drawList, const ImageStyle& style, const glm::vec4& color, const glm::vec2& pos, const glm::vec2& size, float scale);

    // Same, FILLED by amount from origin regardless of the style's mode (progress bars, slider fills)
    void DrawFilledImage(DrawList& drawList, const ImageStyle& style, const glm::vec4& color, const glm::vec2& pos, const glm::vec2& size, float scale, float amount, FillOrigin origin);

    // Plain rect (rounded when radius > 0), pixels
    void DrawRect(DrawList& drawList, const glm::vec2& min, const glm::vec2& max, const glm::vec4& color, float radiusPixels = 0.0f);

    // Single line width in font (em) units, same glyph advance + kerning rules as UIOverlayPass (multiply by the font size in pixels)
    float MeasureText(const Font* font, std::string_view text);

    /*
    * Rect transform (all values in UI units, multiplied by the canvas scale to get pixels):
    *   AnchorMin/AnchorMax : Normalized position inside the parent rect. Equal = point anchor, different = the widget stretches with the parent on that axis
    *   Pivot               : Normalized point of the widget that sits on the anchor (0,0 = top left, 1,1 = bottom right)
    *   Offset              : Position of the pivot relative to the anchor
    *   Size                : Size of the widget. On a stretched axis it is added to the stretched size (negative values inset the widget)
    *
    * Pointer events carry positions in UI pixels (render target, (0,0) = top left), the space of GetGlobalBounds()
    */
    class Widget : public RefCounted
    {
    public:
        virtual ~Widget() = default;

        // Hierarchy
        Vector<Ref<Widget>>& GetChildren() { return mChildren; }
        const Vector<Ref<Widget>>& GetChildren() const { return mChildren; }
        void AddChild(Ref<Widget> child); // Re-parents the child if it already has a parent, adding the same child twice is no-op
        void RemoveChild(Widget* child);
        Widget* GetParent() const { return mParent; }
        Widget* FindChild(const String& name, bool recursive = true); // Depth first, nullptr if not found

        // Identity/State
        void SetName(const String& name) { mName = name; }
        const String& GetName() const { return mName; }
        void SetVisible(bool visible) { mVisible = visible; }
        bool IsVisible() const { return mVisible; }
        void SetInteractable(bool interactable) { mIsInteractable = interactable; }
        bool IsInteractable() const { return mIsInteractable; }

        // Rect transform
        void SetAnchor(float x, float y) { mAnchorMin = mAnchorMax = { x, y }; } // Point anchor
        void SetAnchorMin(float x, float y) { mAnchorMin = { x, y }; }
        void SetAnchorMax(float x, float y) { mAnchorMax = { x, y }; }
        void SetPivot(float x, float y) { mPivot = { x, y }; }
        void SetOffset(float x, float y) { mLocalOffset = { x, y }; }
        void SetSize(float w, float h) { mSize = { w, h }; }
        void SetColor(const glm::vec4& color) { mColor = color; }

        glm::vec2 GetAnchor() const { return mAnchorMin; }
        glm::vec2 GetAnchorMin() const { return mAnchorMin; }
        glm::vec2 GetAnchorMax() const { return mAnchorMax; }
        glm::vec2 GetPivot() const { return mPivot; }
        glm::vec2 GetOffset() const { return mLocalOffset; }
        glm::vec2 GetSize() const { return mSize; }
        glm::vec4 GetColor() const { return mColor; }

        // Layout, recomputed once per frame by UI::Manager (cached so hit testing and drawing don't walk up the hierarchy)
        void LayoutAsRoot(const glm::vec2& screenSize, float scale);  // Root always covers the whole canvas
        virtual void UpdateLayout(const glm::vec2& parentPos, const glm::vec2& parentSize, float scale);
        void LayoutAt(const glm::vec2& pos, const glm::vec2& size, float scale); // Explicit rect in pixels (layout groups, lists), then the children
        void TranslateLayout(const glm::vec2& delta);                             // Moves the cached layout of the whole subtree (pixels)
        virtual glm::vec2 GetPreferredSize() const { return mSize; }             // UI units, what layout groups give this widget
        Pair<glm::vec2, glm::vec2> GetGlobalBounds() const { return { mGlobalPos, mGlobalSize }; } // Returns [GlobalPosition, GlobalSize] in Pixels; (0,0) = top left of the viewport
        float GetLayoutScale() const { return mLayoutScale; }
        bool ContainsPoint(float x, float y) const { return x >= mGlobalPos.x && x <= mGlobalPos.x + mGlobalSize.x && y >= mGlobalPos.y && y <= mGlobalPos.y + mGlobalSize.y; }

        // Events
        void SetOnClick(Ref<IEventCallback> callback) { mOnClick = callback; mIsInteractable = true; }
        void SetOnHoverEnter(Ref<IEventCallback> callback) { mOnHoverEnter = callback; mIsInteractable = true; }
        void SetOnHoverExit(Ref<IEventCallback> callback) { mOnHoverExit = callback; mIsInteractable = true; }
        void SetOnValueChanged(Ref<IEventCallback> callback) { mOnValueChanged = callback; } // Toggle, Slider, InputField, Dropdown, TabView, ScrollView
        bool IsHovered() const { return mIsHovered; }
        bool IsPressed() const { return mIsPressed; }

        virtual void GenerateDrawCommands(DrawList& drawList)
        {
            SURGE_PROFILE_FUNC("Widget::GenerateDrawCommands");

            // (Rid) Child must call the below function in the ovveride to draw its children
            GenerateChildDrawCommands(drawList);
        }

        // Recursive Hit Test, invisible widgets & their children are skipped
        virtual Widget* HitTest(float x, float y);

        // Breaks the widget <-> callback <-> Lua closure reference cycles, call before dropping a hierarchy
        virtual void Destroy();

        virtual WidgetType GetType() const { return WidgetType::BASE_WIDGET; }

        // Pointer, called by UI::Manager. Each pointer (mouse, every finger) presses its own widget
        virtual void OnMouseEnter() { mIsHovered = true; if(mOnHoverEnter) mOnHoverEnter->Invoke(); }
        virtual void OnMouseExit() { mIsHovered = false; mIsPressed = false; if(mOnHoverExit) mOnHoverExit->Invoke(); }
        virtual void OnPointerDown(const glm::vec2&) { mIsPressed = true; }
        virtual void OnPointerDrag(const glm::vec2&) {}
        virtual void OnPointerUp(const glm::vec2&) { if(mIsPressed && mOnClick) mOnClick->Invoke(); mIsPressed = false; } // Released over this widget
        virtual void CancelPress() { mIsPressed = false; } // Released somewhere else, no click
        virtual bool WantsDrag() const { return false; }                             // Drags belong to this widget (sliders, joysticks), never handed to a scroll view
        virtual bool CanTakeOverDrag(const glm::vec2&) const { return false; }       // A scroll view takes over a child's press once it turns into a drag along its axis
        virtual bool OnScroll(float) { return false; }                               // Mouse wheel notches, true when consumed

        // Keyboard focus (InputField)
        virtual bool IsFocusable() const { return false; }
        virtual void OnFocus() {}
        virtual void OnBlur() {}
        virtual bool OnKeyPressed(Key, bool /*ctrl*/, bool /*shift*/) { return false; }
        virtual void OnTextInput(Uint /*codepoint*/) {}

        // Overlay: drawn above every canvas while registered with UI::Manager::OpenOverlay (Dropdown list, Modal)
        virtual void GenerateOverlayDrawCommands(DrawList&) {}
        virtual Widget* OverlayHitTest(float, float) { return nullptr; }
        virtual bool IsOverlayModal() const { return false; } // Blocks input to everything below
        virtual void OnOverlayDismiss() {}                    // Tap outside or Back

        // UI Editor preview: popups draw in place, lists show placeholder items
        virtual void SetDesignPreview(bool) {}

    protected:
        void ComputeRect(const glm::vec2& parentPos, const glm::vec2& parentSize, float scale);
        virtual void LayoutChildren(); // Default: every child relative to this rect

        void GenerateChildDrawCommands(DrawList& drawList)
        {
            for(auto& child : mChildren)
            {
                if(child->IsVisible())
                    child->GenerateDrawCommands(drawList);
            }
        }

        Widget* HitTestChildren(float x, float y);
        void NotifyValueChanged() { if(mOnValueChanged) mOnValueChanged->Invoke(); }

    protected:
        String mName;

        glm::vec2 mAnchorMin { 0.0f, 0.0f };
        glm::vec2 mAnchorMax { 0.0f, 0.0f };
        glm::vec2 mPivot { 0.0f, 0.0f };
        glm::vec2 mLocalOffset { 0.0f, 0.0f };
        glm::vec2 mSize { 100.0f, 100.0f };
        glm::vec4 mColor = { 1.0f, 1.0f, 1.0f, 1.0f };

        // Cached layout (pixels)
        glm::vec2 mGlobalPos { 0.0f, 0.0f };
        glm::vec2 mGlobalSize { 0.0f, 0.0f };
        float mLayoutScale = 1.0f;

        Widget* mParent = nullptr;
        Vector<Ref<Widget>> mChildren;

        bool mVisible = true;
        bool mIsInteractable = false;
        bool mIsHovered = false;
        bool mIsPressed = false;

        Ref<IEventCallback> mOnClick;
        Ref<IEventCallback> mOnHoverEnter;
        Ref<IEventCallback> mOnHoverExit;
        Ref<IEventCallback> mOnValueChanged;
    };

    class Image : public Widget
    {
    public:
        Image(ImageHandle textureId = ImageHandle::Invalid()) { mStyle.Texture = textureId; }

        // textureAsset keeps the owning Texture2D alive for as long as the widget references its RHI image
        void SetTexture(ImageHandle textureId, Ref<Asset> textureAsset = nullptr) { mStyle.SetTexture(textureId, textureAsset); }
        ImageHandle GetTexture() const { return mStyle.Texture; }
        ImageStyle& GetStyle() { return mStyle; }

        virtual WidgetType GetType() const override { return WidgetType::IMAGE; }
        virtual void GenerateDrawCommands(DrawList& drawList) override
        {
            DrawImage(drawList, mStyle, mColor, mGlobalPos, mGlobalSize, mLayoutScale);
            GenerateChildDrawCommands(drawList);
        }

    protected:
        ImageStyle mStyle;
    };

    /*
    * Text is laid out inside its rect: TextAlignment picks left/center/right edge, TextVerticalAlignment the top/center/bottom edge.
    * With the default size of (0, 0) the rect collapses to a point, so the text is aligned around its anchor point (the original behaviour),
    * a non zero width wraps (WRAP) or cuts the line with "..." (TRUNCATE) at that width
    */
    class Text final : public Widget
    {
    public:
        Text(const String& text, AssetID fontAsset);
        Text(const String& text, Ref<Font> font);

        virtual WidgetType GetType() const override { return WidgetType::TEXT; }
        virtual void GenerateDrawCommands(DrawList& drawList) override;

        void SetText(const String& text) { mText = text; }
        void SetFont(Ref<Font> font) { mFontAsset = font; }
        void SetFontSize(float size) { mFontSize = size; }
        void SetTextAlignment(TextAlignment alignment) { mTextAlignment = alignment; }
        void SetTextVAlignment(TextVerticalAlignment alignment) { mTextVerticalAlignment = alignment; }
        void SetWordWrap(bool wrap) { mOverflow = wrap ? TextOverflow::WRAP : TextOverflow::NO_WRAP; }
        void SetOverflow(TextOverflow overflow) { mOverflow = overflow; }

        Ref<Font> GetFontAsset() const { return mFontAsset; }
        String& GetTextBuffer() { return mText; }
        float GetFontSize() const { return mFontSize; }
        TextAlignment GetTextAlignment() const { return mTextAlignment; }
        TextVerticalAlignment GetTextVAlignment() const { return mTextVerticalAlignment; }
        bool GetWordWrap() const { return mOverflow == TextOverflow::WRAP; }
        TextOverflow GetOverflow() const { return mOverflow; }

    private:
        const String& GetDisplayText(float maxWidthEm); // TRUNCATE: cached cut version of mText

    protected:
        String mText;
        TextAlignment mTextAlignment = TextAlignment::LEFT;
        TextVerticalAlignment mTextVerticalAlignment = TextVerticalAlignment::CENTER;
        float mFontSize = 18.0f;
        TextOverflow mOverflow = TextOverflow::WRAP;
        Ref<Font> mFontAsset;

        // TRUNCATE cache, only rebuilt when the text, font or width changes
        String mTruncatedText;
        String mTruncatedSource;
        float mTruncatedWidth = -1.0f;
        const Font* mTruncatedFont = nullptr;
    };

    class Button final : public Image
    {
    public:
        // Pass the background texture, the string, and the font
        Button(const String& text, AssetID fontAsset, ImageHandle textureId = ImageHandle::Invalid())
            : Image(textureId)
        {
            Init(Ref<Text>::Create(text, fontAsset));
        }

        Button(const String& text, Ref<Font> font, ImageHandle textureId = ImageHandle::Invalid())
            : Image(textureId)
        {
            Init(Ref<Text>::Create(text, font));
        }

        void Destroy() override
        {
            if(mTextWidget)
            {
                mTextWidget->Destroy();
                mTextWidget = nullptr;
            }
            UI::Widget::Destroy(); // Call base class destroy to clear mChildren
        }

        Ref<Text> GetTextWidget() { return mTextWidget; }
        void SetText(const String& text) { if(mTextWidget) mTextWidget->SetText(text); }

        // The label is a zero sized text anchored inside the button, so it never wraps and stays aligned when the button is resized
        void SetTextAlignment(TextAlignment align, float paddingX = 15.0f);
        void SetTextVAlignment(TextVerticalAlignment alignment, float paddingY = 15.0f);

        virtual WidgetType GetType() const override { return WidgetType::BUTTON; }
        virtual void GenerateDrawCommands(DrawList& drawList) override
        {
            if(mIsPressed)      mColor = PressedColor;
            else if(mIsHovered) mColor = HoverColor;
            else                mColor = NormalColor;

            // Calls GenerateDrawCommands on mTextWidget(child) recursively so it draws on top!
            Image::GenerateDrawCommands(drawList);
        }

    public:
        glm::vec4 NormalColor = { 1.0f, 1.0f, 1.0f, 1.0f };
        glm::vec4 HoverColor = { 0.8f, 0.8f, 0.8f, 1.0f };
        glm::vec4 PressedColor = { 0.5f, 0.5f, 0.5f, 1.0f };

    private:
        void Init(Ref<Text> textWidget);

    private:
        Ref<Text> mTextWidget;
    };

    class ImageButton final : public Image
    {
    public:
        ImageButton(const ImageHandle& textureId = ImageHandle::Invalid())
            : Image(textureId)
        {
            mIsInteractable = true;
            SetSize(200.0f, 50.0f);
        }

        virtual WidgetType GetType() const  override{ return WidgetType::IMAGE_BUTTON; }
        virtual void GenerateDrawCommands(DrawList& drawList) override
        {
            if(mIsPressed)      mColor = PressedColor;
            else if(mIsHovered) mColor = HoverColor;
            else                mColor = NormalColor;

            Image::GenerateDrawCommands(drawList);
        }

    public:
        glm::vec4 NormalColor = { 1.0f, 1.0f, 1.0f, 1.0f };
        glm::vec4 HoverColor = { 0.8f, 0.8f, 0.8f, 1.0f };
        glm::vec4 PressedColor = { 0.5f, 0.5f, 0.5f, 1.0f };
    };

    // Background + fill, Value in Min..Max. Not interactable (health, XP, loading)
    class ProgressBar final : public Widget
    {
    public:
        ProgressBar();

        virtual WidgetType GetType() const override { return WidgetType::PROGRESS_BAR; }
        virtual void GenerateDrawCommands(DrawList& drawList) override;

        void SetValue(float value) { mValue = glm::clamp(value, glm::min(Min, Max), glm::max(Min, Max)); }
        float GetValue() const { return mValue; }
        float GetNormalizedValue() const { return Max != Min ? glm::clamp((mValue - Min) / (Max - Min), 0.0f, 1.0f) : 0.0f; }

    public:
        float Min = 0.0f;
        float Max = 1.0f;
        FillOrigin Direction = FillOrigin::LEFT;
        float FillPadding = 0.0f; // UI units, inset of the fill inside the background
        ImageStyle Background;
        ImageStyle Fill;
        glm::vec4 BackgroundColor = { 0.15f, 0.16f, 0.19f, 1.0f };
        glm::vec4 FillColor = { 0.25f, 0.75f, 0.4f, 1.0f };

    private:
        float mValue = 0.5f;
    };

    /*
    * Checkbox (box + check mark) or Switch (track + sliding knob), with an optional label on the right
    * Toggles under the same parent sharing a non empty Group act as radio buttons: turning one on turns the others off
    */
    class Toggle final : public Widget
    {
    public:
        Toggle(const String& label, Ref<Font> font);

        virtual WidgetType GetType() const override { return WidgetType::TOGGLE; }
        virtual void GenerateDrawCommands(DrawList& drawList) override;
        virtual void OnPointerUp(const glm::vec2& pos) override;
        virtual void Destroy() override;

        void SetOn(bool on, bool notify = true);
        bool IsOn() const { return mIsOn; }
        Ref<Text> GetLabel() { return mLabel; }

    public:
        ToggleStyle Style = ToggleStyle::CHECKBOX;
        String Group;
        bool AllowSwitchOff = false; // Grouped toggles only: tapping the active one turns it off
        ImageStyle Box;   // Checkbox box / switch track
        ImageStyle Check; // Checkbox mark / switch track when on
        ImageStyle Knob;  // Switch knob
        glm::vec4 BoxColor = { 0.22f, 0.23f, 0.27f, 1.0f };
        glm::vec4 CheckColor = { 0.25f, 0.6f, 1.0f, 1.0f };
        glm::vec4 KnobColor = { 1.0f, 1.0f, 1.0f, 1.0f };

    protected:
        virtual void LayoutChildren() override;

    private:
        void GetBoxRect(glm::vec2& outPos, glm::vec2& outSize) const;

    private:
        Ref<Text> mLabel;
        bool mIsOn = false;
        float mKnobT = 0.0f; // Animated 0 (off) .. 1 (on)
    };

    // Value in Min..Max picked by dragging a handle along a track, tapping the track jumps there
    class Slider final : public Widget
    {
    public:
        Slider();

        virtual WidgetType GetType() const override { return WidgetType::SLIDER; }
        virtual void GenerateDrawCommands(DrawList& drawList) override;
        virtual void OnPointerDown(const glm::vec2& pos) override;
        virtual void OnPointerDrag(const glm::vec2& pos) override;
        virtual void OnPointerUp(const glm::vec2& pos) override;
        virtual void CancelPress() override { mIsPressed = false; mDragging = false; }
        virtual bool WantsDrag() const override { return true; }

        void SetValue(float value, bool notify = true);
        float GetValue() const { return mValue; }
        float GetNormalizedValue() const { return Max != Min ? glm::clamp((mValue - Min) / (Max - Min), 0.0f, 1.0f) : 0.0f; }

    public:
        float Min = 0.0f;
        float Max = 1.0f;
        bool WholeNumbers = false;
        FillOrigin Direction = FillOrigin::LEFT;
        float TrackThickness = 12.0f; // UI units across the track, 0 = the whole rect
        float HandleSize = 40.0f;     // UI units
        ImageStyle Track;
        ImageStyle Fill;
        ImageStyle Handle;
        glm::vec4 TrackColor = { 0.22f, 0.23f, 0.27f, 1.0f };
        glm::vec4 FillColor = { 0.25f, 0.6f, 1.0f, 1.0f };
        glm::vec4 HandleColor = { 1.0f, 1.0f, 1.0f, 1.0f };

    private:
        bool IsHorizontal() const { return Direction == FillOrigin::LEFT || Direction == FillOrigin::RIGHT; }
        void SetValueFromPointer(const glm::vec2& pos);

    private:
        float mValue = 0.5f;
        bool mDragging = false;
    };

    /*
    * Virtual stick. GetDirection() is -1..1 per axis with +Y up, the dead zone is removed and the magnitude rescaled to 0..1
    * Every finger presses its own widget, so two sticks (move + aim) work at the same time
    */
    class Joystick final : public Widget
    {
    public:
        Joystick();

        virtual WidgetType GetType() const override { return WidgetType::JOYSTICK; }
        virtual void GenerateDrawCommands(DrawList& drawList) override;
        virtual void OnPointerDown(const glm::vec2& pos) override;
        virtual void OnPointerDrag(const glm::vec2& pos) override;
        virtual void OnPointerUp(const glm::vec2&) override { Release(); }
        virtual void CancelPress() override { Release(); }
        virtual void OnMouseExit() override { mIsHovered = false; } // Keeps tracking the finger outside the rect
        virtual bool WantsDrag() const override { return true; }

        glm::vec2 GetDirection() const { return mDirection; }
        float GetMagnitude() const { return glm::length(mDirection); }
        bool IsActive() const { return mActive; }

    public:
        JoystickMode Mode = JoystickMode::FIXED;
        float BaseSize = 0.0f;   // UI units, 0 = fit the rect
        float HandleSize = 0.0f; // UI units, 0 = 45% of the base
        float DeadZone = 0.1f;   // 0..1 of the range
        float HandleRange = 1.0f; // Max knob travel, in base radii
        bool HideWhenIdle = false;
        ImageStyle Base;
        ImageStyle Knob;
        glm::vec4 BaseColor = { 1.0f, 1.0f, 1.0f, 0.25f };
        glm::vec4 KnobColor = { 1.0f, 1.0f, 1.0f, 0.85f };

    private:
        float GetBaseRadius() const;
        void Release();

    private:
        glm::vec2 mCenter = { 0.0f, 0.0f };     // Pixels, where the base sits while active
        glm::vec2 mKnobOffset = { 0.0f, 0.0f }; // Pixels from mCenter
        glm::vec2 mDirection = { 0.0f, 0.0f };
        bool mActive = false;
    };

    // Busy indicator: dots on a circle with a rotating fade (iOS/Android style). Color = dot color
    class Spinner final : public Widget
    {
    public:
        Spinner() { mSize = { 64.0f, 64.0f }; }

        virtual WidgetType GetType() const override { return WidgetType::SPINNER; }
        virtual void GenerateDrawCommands(DrawList& drawList) override;

    public:
        int DotCount = 10;
        float DotSize = 0.2f; // Fraction of the diameter
        float Speed = 1.0f;   // Revolutions per second
    };

    // Insets its children by the device safe area (notch, rounded corners, status/navigation bars) on the chosen edges
    class SafeArea final : public Widget
    {
    public:
        enum Edge : int { EDGE_LEFT = BIT(0), EDGE_TOP = BIT(1), EDGE_RIGHT = BIT(2), EDGE_BOTTOM = BIT(3), EDGE_ALL = 15 };

        virtual WidgetType GetType() const override { return WidgetType::SAFE_AREA; }

    public:
        int Edges = EDGE_ALL;

    protected:
        virtual void LayoutChildren() override;
    };

    /*
    * Arranges its visible children in a row, a column or a grid (their anchors/offsets are ignored, their Size is used)
    * FitContent resizes the group to its content on non stretched axes, e.g. the content of a ScrollView
    */
    class LayoutGroup final : public Widget
    {
    public:
        virtual WidgetType GetType() const override { return WidgetType::LAYOUT_GROUP; }
        virtual void UpdateLayout(const glm::vec2& parentPos, const glm::vec2& parentSize, float scale) override;
        virtual glm::vec2 GetPreferredSize() const override;

    public:
        LayoutDirection Direction = LayoutDirection::VERTICAL;
        glm::vec4 Padding = { 0.0f, 0.0f, 0.0f, 0.0f }; // UI units: left, top, right, bottom
        glm::vec2 Spacing = { 10.0f, 10.0f };
        glm::vec2 CellSize = { 100.0f, 100.0f };       // GRID
        int Columns = 0;                               // GRID, 0 = as many as fit
        glm::vec2 ChildAlignment = { 0.0f, 0.0f };     // 0 = left/top, 0.5 = center, 1 = right/bottom
        bool ControlChildWidth = false;                // Children take the full width (vertical) or share it equally (horizontal)
        bool ControlChildHeight = false;
        bool FitContent = false;

    protected:
        virtual void LayoutChildren() override;

    private:
        int GetColumnCount(float innerWidth) const; // UI units
        glm::vec2 ComputeContentSize(float innerWidth) const; // UI units, padding included
    };

    // A tab bar (top or bottom) and one page per direct child, the child's name is the tab label. Only the active page is laid out, drawn and hit
    class TabView final : public Widget
    {
    public:
        TabView();

        virtual WidgetType GetType() const override { return WidgetType::TAB_VIEW; }
        virtual void GenerateDrawCommands(DrawList& drawList) override;
        virtual Widget* HitTest(float x, float y) override;
        virtual void OnPointerDown(const glm::vec2& pos) override;
        virtual void OnPointerUp(const glm::vec2& pos) override;

        void SetActiveTab(int index, bool notify = true);
        int GetActiveTab() const { return mActiveTab; }

    public:
        bool TabsAtBottom = false;
        float TabBarHeight = 90.0f; // UI units
        Ref<Font> TextFont;
        float FontSize = 30.0f;
        ImageStyle Bar;
        ImageStyle ActiveTabStyle;
        glm::vec4 BarColor = { 0.12f, 0.13f, 0.16f, 1.0f };
        glm::vec4 ActiveTabColor = { 0.25f, 0.6f, 1.0f, 1.0f };
        glm::vec4 TextColor = { 0.75f, 0.77f, 0.82f, 1.0f };
        glm::vec4 ActiveTextColor = { 1.0f, 1.0f, 1.0f, 1.0f };

    protected:
        virtual void LayoutChildren() override;

    private:
        void GetBarRect(glm::vec2& outPos, glm::vec2& outSize) const;
        int TabAt(const glm::vec2& pos) const; // -1 outside the bar

    private:
        int mActiveTab = 0;
        int mPressedTab = -1;
    };

} // namespace Surge::UI
