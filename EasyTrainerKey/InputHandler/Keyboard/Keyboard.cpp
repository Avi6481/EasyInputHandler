#include "InputHandler/Keyboard/Keyboard.h"
#include "InputHandler/Window.h"

#include <atomic>

namespace EasyTrainer::Input::Keyboard
{
    constexpr int FIRST_KEY = 0x01;
    constexpr int LAST_KEY = 0xFE;
    constexpr uint64_t SCAN_CACHE_MS = 10;

    static std::atomic<int> s_LastPressed = 0;
    static std::atomic<uint64_t> s_LastScan = 0;

    static int ScanPressedKey()
    {
        if (!Window::IsGameFocused())
            return 0;

        for (int key = FIRST_KEY; key <= LAST_KEY; key++)
        {
            if ((GetAsyncKeyState(key) & 0x8000) != 0)
                return key;
        }
        return 0;
    }

    static int FirstPressedKey()
    {
        const uint64_t now = GetTickCount64();
        if (now - s_LastScan < SCAN_CACHE_MS)
            return s_LastPressed;

        s_LastPressed = ScanPressedKey();
        s_LastScan = now;
        return s_LastPressed;
    }

    bool IsKeyDown(int virtualKey)
    {
        if (!Window::IsGameFocused())
            return false;
        return (GetAsyncKeyState(virtualKey) & 0x8000) != 0;
    }

    void IsKeyPressed(RED4ext::IScriptable*, RED4ext::CStackFrame* frame, bool* out, int64_t)
    {
        int32_t key = 0;
        RED4ext::GetParameter(frame, &key);
        frame->code++;

        if (out) *out = IsKeyDown(key);
    }

    void IsKeyboardActive(RED4ext::IScriptable*, RED4ext::CStackFrame* frame, bool* out, int64_t)
    {
        frame->code++;
        if (out) *out = FirstPressedKey() != 0;
    }

    void GetPressedKey(RED4ext::IScriptable*, RED4ext::CStackFrame* frame, int32_t* out, int64_t)
    {
        frame->code++;
        if (out) *out = FirstPressedKey();
    }

    void GetPressedKeys(RED4ext::IScriptable*, RED4ext::CStackFrame* frame, RED4ext::DynArray<int32_t>* out, int64_t)
    {
        frame->code++;
        if (!out || !Window::IsGameFocused())
            return;

        for (int key = FIRST_KEY; key <= LAST_KEY; key++)
        {
            if ((GetAsyncKeyState(key) & 0x8000) != 0)
                out->PushBack(key);
        }
    }
}
