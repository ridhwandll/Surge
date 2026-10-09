// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Core/Input/InputState.hpp"
#include "Surge/Core/Events/Event.hpp"
#include "Surge/Core/Vector.hpp"
#include <glm/glm.hpp>
#include <bitset>

namespace Surge::InputState
{
    namespace
    {
        constexpr int64_t MOUSE_POINTER_ID = -1; // Pointer id of the touch emulated by the left mouse button
        constexpr Uint MAX_KEYS = 256;
        constexpr Uint MOUSE_BUTTONS = 3;
        constexpr float STICK_DEADZONE = 0.2f;
        constexpr float TRIGGER_DEADZONE = 0.05f;

        struct TouchSlot
        {
            Touch Data;
            int64_t PointerID = 0;
            bool PendingEnd = false;   // Began and ended between two frames: report BEGAN now, ENDED next frame
            bool PendingCancel = false;
        };

        struct GamepadState
        {
            bool Connected = false;
            std::bitset<static_cast<size_t>(GamepadButton::COUNT)> Down, Pressed, Released;
            float Axes[static_cast<size_t>(GamepadAxis::COUNT)] = {};
        };

        struct State
        {
            std::bitset<MAX_KEYS> KeysDown, KeysPressed, KeysReleased;
            std::bitset<MOUSE_BUTTONS> MouseDown, MousePressed, MouseReleased;
            glm::vec2 MousePosition = { 0.0f, 0.0f };
            float ScrollDelta = 0.0f;

            Vector<TouchSlot> Touches;
            bool MouseTouchEmulation = true;
            bool TouchSupported = false;

            GamepadState Gamepads[Input::MAX_GAMEPADS];
            bool BackButtonExitsApp = true;
        };
        State sState;

        int MouseIndex(MouseCode button)
        {
            switch(button)
            {
                case static_cast<MouseCode>(Mouse::ButtonLeft): return 0;
                case static_cast<MouseCode>(Mouse::ButtonRight): return 1;
                case static_cast<MouseCode>(Mouse::ButtonMiddle): return 2;
                default: return -1;
            }
        }

        bool IsTouchActive(const TouchSlot& slot)
        {
            return !slot.PendingEnd && slot.Data.Phase != TouchPhase::ENDED && slot.Data.Phase != TouchPhase::CANCELED;
        }

        TouchSlot* FindActiveSlot(int64_t pointerID)
        {
            for(TouchSlot& slot : sState.Touches)
            {
                if(slot.PointerID == pointerID && IsTouchActive(slot))
                    return &slot;
            }
            return nullptr;
        }

        int32_t AllocateTouchID()
        {
            // Smallest index not used by any touch still listed (ended ones included, an ID is never reused within a frame)
            for(int32_t id = 0;; id++)
            {
                bool used = false;
                for(const TouchSlot& slot : sState.Touches)
                {
                    if(slot.Data.ID == id)
                    {
                        used = true;
                        break;
                    }
                }
                if(!used)
                    return id;
            }
        }

        void TouchMove(int64_t pointerID, float x, float y, float pressure)
        {
            TouchSlot* slot = FindActiveSlot(pointerID);
            if(!slot)
                return;

            const glm::vec2 position = { x, y };
            slot->Data.Delta += position - slot->Data.Position;
            slot->Data.Position = position;
            slot->Data.Pressure = pressure;
            if(slot->Data.Phase != TouchPhase::BEGAN)
                slot->Data.Phase = TouchPhase::MOVED;
        }

        void TouchDown(int64_t pointerID, float x, float y, float pressure, bool isMouse)
        {
            if(FindActiveSlot(pointerID)) // Missed release, keep the contact alive
            {
                TouchMove(pointerID, x, y, pressure);
                return;
            }

            TouchSlot slot;
            slot.PointerID = pointerID;
            slot.Data.ID = AllocateTouchID();
            slot.Data.Phase = TouchPhase::BEGAN;
            slot.Data.Position = slot.Data.StartPosition = { x, y };
            slot.Data.Pressure = pressure;
            slot.Data.IsMouse = isMouse;
            sState.Touches.push_back(slot);
        }

        void TouchUp(int64_t pointerID, float x, float y, bool canceled)
        {
            TouchSlot* slot = FindActiveSlot(pointerID);
            if(!slot)
                return;

            const glm::vec2 position = { x, y };
            slot->Data.Delta += position - slot->Data.Position;
            slot->Data.Position = position;
            if(slot->Data.Phase == TouchPhase::BEGAN)
            {
                slot->PendingEnd = true;
                slot->PendingCancel = canceled;
            }
            else
                slot->Data.Phase = canceled ? TouchPhase::CANCELED : TouchPhase::ENDED;
        }

        void CancelAllTouches()
        {
            for(TouchSlot& slot : sState.Touches)
            {
                if(!IsTouchActive(slot))
                    continue;

                if(slot.Data.Phase == TouchPhase::BEGAN)
                {
                    slot.PendingEnd = true;
                    slot.PendingCancel = true;
                }
                else
                    slot.Data.Phase = TouchPhase::CANCELED;
            }
        }

        void SetMouseButton(MouseCode button, bool down)
        {
            const int index = MouseIndex(button);
            if(index < 0)
                return;

            if(down && !sState.MouseDown[index])
                sState.MousePressed[index] = true;
            else if(!down && sState.MouseDown[index])
                sState.MouseReleased[index] = true;
            sState.MouseDown[index] = down;
        }

        void SetKey(KeyCode key, bool down)
        {
            if(key >= MAX_KEYS)
                return;

            if(down && !sState.KeysDown[key])
                sState.KeysPressed[key] = true;
            else if(!down && sState.KeysDown[key])
                sState.KeysReleased[key] = true;
            sState.KeysDown[key] = down;
        }
    }

    void BeginFrame(float deltaSeconds)
    {
        sState.KeysPressed.reset();
        sState.KeysReleased.reset();
        sState.MousePressed.reset();
        sState.MouseReleased.reset();
        sState.ScrollDelta = 0.0f;
        for(GamepadState& pad : sState.Gamepads)
        {
            pad.Pressed.reset();
            pad.Released.reset();
        }

        // Touches that ended last frame are gone, the rest becomes stationary until the platform reports movement
        Vector<TouchSlot>& touches = sState.Touches;
        for(size_t i = 0; i < touches.size();)
        {
            TouchSlot& slot = touches[i];
            if(slot.Data.Phase == TouchPhase::ENDED || slot.Data.Phase == TouchPhase::CANCELED)
            {
                touches.erase(touches.begin() + static_cast<std::ptrdiff_t>(i));
                continue;
            }

            slot.Data.Delta = { 0.0f, 0.0f };
            slot.Data.Duration += deltaSeconds;
            if(slot.PendingEnd)
            {
                slot.Data.Phase = slot.PendingCancel ? TouchPhase::CANCELED : TouchPhase::ENDED;
                slot.PendingEnd = false;
            }
            else
                slot.Data.Phase = TouchPhase::STATIONARY;
            i++;
        }

        PlatformBeginFrame();
    }

    void OnEvent(Event& e)
    {
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<KeyPressedEvent>([](KeyPressedEvent& ev) { SetKey(static_cast<KeyCode>(ev.GetKeyCode()), true); });
        dispatcher.Dispatch<KeyReleasedEvent>([](KeyReleasedEvent& ev) { SetKey(static_cast<KeyCode>(ev.GetKeyCode()), false); });

        dispatcher.Dispatch<MouseMovedEvent>([](MouseMovedEvent& ev) {
            sState.MousePosition = { ev.GetX(), ev.GetY() };
            if(!ev.IsFromTouch())
                TouchMove(MOUSE_POINTER_ID, ev.GetX(), ev.GetY(), 1.0f);
        });
        dispatcher.Dispatch<MouseButtonPressedEvent>([](MouseButtonPressedEvent& ev) {
            sState.MousePosition = { ev.GetX(), ev.GetY() };
            SetMouseButton(ev.GetMouseButton(), true);
            if(!ev.IsFromTouch() && sState.MouseTouchEmulation && ev.GetMouseButton() == static_cast<MouseCode>(Mouse::ButtonLeft))
                TouchDown(MOUSE_POINTER_ID, ev.GetX(), ev.GetY(), 1.0f, true);
        });
        dispatcher.Dispatch<MouseButtonReleasedEvent>([](MouseButtonReleasedEvent& ev) {
            sState.MousePosition = { ev.GetX(), ev.GetY() };
            SetMouseButton(ev.GetMouseButton(), false);
            if(!ev.IsFromTouch() && ev.GetMouseButton() == static_cast<MouseCode>(Mouse::ButtonLeft))
                TouchUp(MOUSE_POINTER_ID, ev.GetX(), ev.GetY(), false);
        });
        dispatcher.Dispatch<MouseScrolledEvent>([](MouseScrolledEvent& ev) { sState.ScrollDelta += ev.GetDelta(); });

        dispatcher.Dispatch<TouchPressedEvent>([](TouchPressedEvent& ev) { TouchDown(ev.GetPointerID(), ev.GetX(), ev.GetY(), ev.GetPressure(), false); });
        dispatcher.Dispatch<TouchMovedEvent>([](TouchMovedEvent& ev) { TouchMove(ev.GetPointerID(), ev.GetX(), ev.GetY(), ev.GetPressure()); });
        dispatcher.Dispatch<TouchReleasedEvent>([](TouchReleasedEvent& ev) { TouchUp(ev.GetPointerID(), ev.GetX(), ev.GetY(), ev.IsCanceled()); });

        dispatcher.Dispatch<WindowLostFocusEvent>([](WindowLostFocusEvent&) {
            // Nothing held can be released anymore once the window lost focus
            for(Uint k = 0; k < MAX_KEYS; k++)
                if(sState.KeysDown[k])
                    SetKey(static_cast<KeyCode>(k), false);
            for(MouseCode b : { MouseCode(1), MouseCode(2), MouseCode(4) })
                SetMouseButton(b, false);
            CancelAllTouches();
        });
    }

    bool IsKeyDown(Key key)
    {
        const KeyCode code = static_cast<KeyCode>(key);
        return code < MAX_KEYS && sState.KeysDown[code];
    }

    bool IsMouseButtonDown(Mouse button)
    {
        const int index = MouseIndex(static_cast<MouseCode>(button));
        return index >= 0 && sState.MouseDown[index];
    }

    glm::vec2 GetMousePosition() { return sState.MousePosition; }

    void SetGamepadConnected(Uint index, bool connected)
    {
        if(index >= Input::MAX_GAMEPADS)
            return;

        GamepadState& pad = sState.Gamepads[index];
        if(pad.Connected && !connected)
            pad = GamepadState();
        pad.Connected = connected;
    }

    void SetGamepadButton(Uint index, GamepadButton button, bool down)
    {
        if(index >= Input::MAX_GAMEPADS || button >= GamepadButton::COUNT)
            return;

        GamepadState& pad = sState.Gamepads[index];
        const size_t b = static_cast<size_t>(button);
        if(down && !pad.Down[b])
            pad.Pressed[b] = true;
        else if(!down && pad.Down[b])
            pad.Released[b] = true;
        pad.Down[b] = down;
        pad.Connected = true;
    }

    void SetGamepadAxis(Uint index, GamepadAxis axis, float value)
    {
        if(index >= Input::MAX_GAMEPADS || axis >= GamepadAxis::COUNT)
            return;

        sState.Gamepads[index].Axes[static_cast<size_t>(axis)] = value;
        sState.Gamepads[index].Connected = true;
    }

    void SetTouchSupported(bool supported) { sState.TouchSupported = supported; }

} // namespace Surge::InputState

namespace Surge
{
    using namespace InputState;

    bool Input::WasKeyPressed(Key key)
    {
        const KeyCode code = static_cast<KeyCode>(key);
        return code < MAX_KEYS && sState.KeysPressed[code];
    }

    bool Input::WasKeyReleased(Key key)
    {
        const KeyCode code = static_cast<KeyCode>(key);
        return code < MAX_KEYS && sState.KeysReleased[code];
    }

    bool Input::WasMouseButtonPressed(Mouse button)
    {
        const int index = MouseIndex(static_cast<MouseCode>(button));
        return index >= 0 && sState.MousePressed[index];
    }

    bool Input::WasMouseButtonReleased(Mouse button)
    {
        const int index = MouseIndex(static_cast<MouseCode>(button));
        return index >= 0 && sState.MouseReleased[index];
    }

    float Input::GetMouseScrollDelta() { return sState.ScrollDelta; }

    Uint Input::GetTouchCount() { return static_cast<Uint>(sState.Touches.size()); }

    const Touch& Input::GetTouch(Uint index)
    {
        static const Touch sInvalid;
        return index < sState.Touches.size() ? sState.Touches[index].Data : sInvalid;
    }

    const Touch* Input::FindTouch(int32_t id)
    {
        for(const TouchSlot& slot : sState.Touches)
        {
            if(slot.Data.ID == id)
                return &slot.Data;
        }
        return nullptr;
    }

    bool Input::IsTouchSupported() { return sState.TouchSupported; }

    void Input::SetMouseTouchEmulation(bool enabled)
    {
        if(!enabled)
            TouchUp(MOUSE_POINTER_ID, sState.MousePosition.x, sState.MousePosition.y, true);
        sState.MouseTouchEmulation = enabled;
    }

    bool Input::IsMouseTouchEmulationEnabled() { return sState.MouseTouchEmulation; }

    bool Input::IsGamepadConnected(Uint index) { return index < MAX_GAMEPADS && sState.Gamepads[index].Connected; }

    bool Input::IsGamepadButtonPressed(Uint index, GamepadButton button)
    {
        return index < MAX_GAMEPADS && button < GamepadButton::COUNT && sState.Gamepads[index].Down[static_cast<size_t>(button)];
    }

    bool Input::WasGamepadButtonPressed(Uint index, GamepadButton button)
    {
        return index < MAX_GAMEPADS && button < GamepadButton::COUNT && sState.Gamepads[index].Pressed[static_cast<size_t>(button)];
    }

    bool Input::WasGamepadButtonReleased(Uint index, GamepadButton button)
    {
        return index < MAX_GAMEPADS && button < GamepadButton::COUNT && sState.Gamepads[index].Released[static_cast<size_t>(button)];
    }

    float Input::GetGamepadAxis(Uint index, GamepadAxis axis)
    {
        if(index >= MAX_GAMEPADS || axis >= GamepadAxis::COUNT)
            return 0.0f;

        const float* axes = sState.Gamepads[index].Axes;
        if(axis == GamepadAxis::LeftTrigger || axis == GamepadAxis::RightTrigger)
        {
            const float v = glm::clamp(axes[static_cast<size_t>(axis)], 0.0f, 1.0f);
            return v < TRIGGER_DEADZONE ? 0.0f : (v - TRIGGER_DEADZONE) / (1.0f - TRIGGER_DEADZONE);
        }

        // Radial deadzone on the stick the axis belongs to, rescaled so the output still covers 0..1
        const bool left = (axis == GamepadAxis::LeftX || axis == GamepadAxis::LeftY);
        const glm::vec2 stick = left ? glm::vec2(axes[static_cast<size_t>(GamepadAxis::LeftX)], axes[static_cast<size_t>(GamepadAxis::LeftY)])
                                     : glm::vec2(axes[static_cast<size_t>(GamepadAxis::RightX)], axes[static_cast<size_t>(GamepadAxis::RightY)]);
        const float length = glm::length(stick);
        if(length < STICK_DEADZONE)
            return 0.0f;

        const float scaled = glm::min(1.0f, (length - STICK_DEADZONE) / (1.0f - STICK_DEADZONE));
        const glm::vec2 result = stick / length * scaled;
        return (axis == GamepadAxis::LeftX || axis == GamepadAxis::RightX) ? result.x : result.y;
    }

    void Input::SetBackButtonExitsApp(bool exits) { sState.BackButtonExitsApp = exits; }
    bool Input::DoesBackButtonExitApp() { return sState.BackButtonExitsApp; }

} // namespace Surge
