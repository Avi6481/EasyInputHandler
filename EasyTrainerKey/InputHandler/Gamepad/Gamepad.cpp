#include "InputHandler/Gamepad/Gamepad.h"
#include "InputHandler/Gamepad/SdlGamepad.h"
#include "InputHandler/Window.h"

#include <algorithm>
#include <cstdlib>
#include <string_view>

namespace EasyTrainer::Input::Gamepad
{
    constexpr int32_t STICK_DEADZONE = 7849;

    static SdlGamepad::State ReadState()
    {
        if (!Window::IsGameFocused())
            return {};
        return SdlGamepad::Poll();
    }

    static int32_t Axis(const SdlGamepad::State& state, SdlGamepad::Axis axis)
    {
        return state.Axes[static_cast<int32_t>(axis)];
    }

    static bool StickDirection(int32_t x, int32_t y, std::string_view direction)
    {
        if (std::abs(x) < STICK_DEADZONE) x = 0;
        if (std::abs(y) < STICK_DEADZONE) y = 0;

        if (direction == "up") return y > 0 && x == 0;
        if (direction == "down") return y < 0 && x == 0;
        if (direction == "left") return x < 0 && y == 0;
        if (direction == "right") return x > 0 && y == 0;
        if (direction == "upleft") return x < 0 && y > 0;
        if (direction == "upright") return x > 0 && y > 0;
        if (direction == "downleft") return x < 0 && y < 0;
        if (direction == "downright") return x > 0 && y < 0;
        return false;
    }

    void IsGamepadButtonPressed(RED4ext::IScriptable*, RED4ext::CStackFrame* frame, bool* out, int64_t)
    {
        int32_t button = 0;
        RED4ext::GetParameter(frame, &button);
        frame->code++;

        if (out) *out = (ReadState().Buttons & static_cast<uint32_t>(button)) != 0;
    }

    void IsGamepadActive(RED4ext::IScriptable*, RED4ext::CStackFrame* frame, bool* out, int64_t)
    {
        frame->code++;
        if (out) *out = ReadState().Buttons != 0;
    }

    void IsGamepadConnected(RED4ext::IScriptable*, RED4ext::CStackFrame* frame, bool* out, int64_t)
    {
        frame->code++;
        if (out) *out = SdlGamepad::Poll().Connected;
    }

    void GetPressedGamepadButton(RED4ext::IScriptable*, RED4ext::CStackFrame* frame, int32_t* out, int64_t)
    {
        frame->code++;
        if (out) *out = static_cast<int32_t>(ReadState().Buttons);
    }

    void GetGamepadButtonName(RED4ext::IScriptable*, RED4ext::CStackFrame* frame, RED4ext::CString* out, int64_t)
    {
        int32_t button = 0;
        RED4ext::GetParameter(frame, &button);
        frame->code++;

        if (out) *out = RED4ext::CString(SdlGamepad::ButtonName(static_cast<uint32_t>(button), SdlGamepad::Poll().Type));
    }

    void GetGamepadType(RED4ext::IScriptable*, RED4ext::CStackFrame* frame, int32_t* out, int64_t)
    {
        frame->code++;
        if (out) *out = static_cast<int32_t>(SdlGamepad::Poll().Type);
    }

    void GetGamepadAxis(RED4ext::IScriptable*, RED4ext::CStackFrame* frame, int32_t* out, int64_t)
    {
        int32_t axis = 0;
        RED4ext::GetParameter(frame, &axis);
        frame->code++;

        if (!out) return;
        *out = 0;
        if (axis < 0 || axis >= static_cast<int32_t>(SdlGamepad::Axis::Count)) return;
        *out = ReadState().Axes[axis];
    }

    void IsLeftStickDirection(RED4ext::IScriptable*, RED4ext::CStackFrame* frame, bool* out, int64_t)
    {
        RED4ext::CString direction;
        RED4ext::GetParameter(frame, &direction);
        frame->code++;

        if (!out) return;
        const auto state = ReadState();
        *out = StickDirection(Axis(state, SdlGamepad::Axis::LeftX), Axis(state, SdlGamepad::Axis::LeftY), direction.c_str());
    }

    void IsRightStickDirection(RED4ext::IScriptable*, RED4ext::CStackFrame* frame, bool* out, int64_t)
    {
        RED4ext::CString direction;
        RED4ext::GetParameter(frame, &direction);
        frame->code++;

        if (!out) return;
        const auto state = ReadState();
        *out = StickDirection(Axis(state, SdlGamepad::Axis::RightX), Axis(state, SdlGamepad::Axis::RightY), direction.c_str());
    }

    void GetLeftTrigger(RED4ext::IScriptable*, RED4ext::CStackFrame* frame, int32_t* out, int64_t)
    {
        frame->code++;
        if (out) *out = Axis(ReadState(), SdlGamepad::Axis::LeftTrigger);
    }

    void GetRightTrigger(RED4ext::IScriptable*, RED4ext::CStackFrame* frame, int32_t* out, int64_t)
    {
        frame->code++;
        if (out) *out = Axis(ReadState(), SdlGamepad::Axis::RightTrigger);
    }

    void GetLeftTriggerNormalized(RED4ext::IScriptable*, RED4ext::CStackFrame* frame, float* out, int64_t)
    {
        frame->code++;
        if (out) *out = static_cast<float>(Axis(ReadState(), SdlGamepad::Axis::LeftTrigger)) / 255.f;
    }

    void GetRightTriggerNormalized(RED4ext::IScriptable*, RED4ext::CStackFrame* frame, float* out, int64_t)
    {
        frame->code++;
        if (out) *out = static_cast<float>(Axis(ReadState(), SdlGamepad::Axis::RightTrigger)) / 255.f;
    }

    void SetGamepadVibration(RED4ext::IScriptable*, RED4ext::CStackFrame* frame, bool* out, int64_t)
    {
        int32_t left = 0;
        int32_t right = 0;
        RED4ext::GetParameter(frame, &left);
        RED4ext::GetParameter(frame, &right);
        frame->code++;

        const auto low = static_cast<uint16_t>(std::clamp(left, 0, 65535));
        const auto high = static_cast<uint16_t>(std::clamp(right, 0, 65535));
        const bool applied = SdlGamepad::Rumble(low, high);
        if (out) *out = applied;
    }
}
