#include "InputHandler/Mouse/Mouse.h"
#include "InputHandler/Messages/Messages.h"
#include "InputHandler/Window.h"

namespace EasyTrainer::Input::Mouse
{
    struct Position
    {
        float X = 0.f;
        float Y = 0.f;
    };

    static Position ReadPosition()
    {
        uint32_t width = 0;
        uint32_t height = 0;
        HWND window = Messages::GetWindowInfo(width, height);

        POINT cursor = {};
        if (!window || !GetCursorPos(&cursor) || !ScreenToClient(window, &cursor))
            return {};

        RECT client = {};
        GetClientRect(window, &client);
        if (client.right <= 0 || client.bottom <= 0 || width == 0 || height == 0)
            return { static_cast<float>(cursor.x), static_cast<float>(cursor.y) };

        return {
            static_cast<float>(cursor.x) * static_cast<float>(width) / static_cast<float>(client.right),
            static_cast<float>(cursor.y) * static_cast<float>(height) / static_cast<float>(client.bottom)
        };
    }

    static Messages::PhysicalButton ToPhysical(int32_t button, bool swapped)
    {
        if (button == 2)
            return Messages::PhysicalButton::Middle;

        const bool left = (button == 0) != swapped;
        return left ? Messages::PhysicalButton::Left : Messages::PhysicalButton::Right;
    }

    static int ToVirtualKey(Messages::PhysicalButton button)
    {
        switch (button)
        {
        case Messages::PhysicalButton::Left: return VK_LBUTTON;
        case Messages::PhysicalButton::Right: return VK_RBUTTON;
        default: return VK_MBUTTON;
        }
    }

    void GetMouseX(RED4ext::IScriptable*, RED4ext::CStackFrame* frame, float* out, int64_t)
    {
        frame->code++;
        if (out) *out = ReadPosition().X;
    }

    void GetMouseY(RED4ext::IScriptable*, RED4ext::CStackFrame* frame, float* out, int64_t)
    {
        frame->code++;
        if (out) *out = ReadPosition().Y;
    }

    void IsMouseButtonDown(RED4ext::IScriptable*, RED4ext::CStackFrame* frame, bool* out, int64_t)
    {
        int32_t button = 0;
        RED4ext::GetParameter(frame, &button);
        frame->code++;

        if (!out) return;
        *out = false;
        if (button < 0 || button > 2) return;

        if (!Window::IsGameFocused())
        {
            Messages::ClearRawButtons();
            return;
        }

        const auto physical = ToPhysical(button, GetSystemMetrics(SM_SWAPBUTTON) != 0);
        *out = (GetAsyncKeyState(ToVirtualKey(physical)) & 0x8000) != 0 || Messages::IsRawButtonDown(physical);
    }

    void ConsumeMouseWheel(RED4ext::IScriptable*, RED4ext::CStackFrame* frame, float* out, int64_t)
    {
        frame->code++;

        const float notches = Messages::ConsumeWheel();
        if (out) *out = Window::IsGameFocused() ? notches : 0.f;
    }

    void LogInputDiagnostics(RED4ext::IScriptable*, RED4ext::CStackFrame* frame, void*, int64_t)
    {
        frame->code++;

        Messages::EnsureInstalled();
        Messages::LogDiagnostics("requested from script");
    }
}
