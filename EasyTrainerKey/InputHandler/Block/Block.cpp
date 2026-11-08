#include "InputHandler/Block/Block.h"
#include "Plugin.h"

#include <Xinput.h>

#include <atomic>
#include <format>

namespace EasyTrainer::Input::Block
{
    constexpr uint64_t HOLD_MS = 250;
    constexpr uint64_t REPEAT_GAP_MS = 1500;
    constexpr int KEY_COUNT = 256;
    constexpr int MOUSE_COUNT = 5;
    constexpr int PAD_BITS = 16;
    constexpr int STICK_COUNT = 2;
    constexpr uint32_t LEFT_TRIGGER = 0x0400;
    constexpr uint32_t RIGHT_TRIGGER = 0x0800;
    constexpr uint32_t TRIGGER_BITS = LEFT_TRIGGER | RIGHT_TRIGGER;
    constexpr USHORT WHEEL_FLAGS = RI_MOUSE_WHEEL | RI_MOUSE_HWHEEL;

    constexpr USHORT MOUSE_DOWN[MOUSE_COUNT] = {
        RI_MOUSE_LEFT_BUTTON_DOWN,
        RI_MOUSE_RIGHT_BUTTON_DOWN,
        RI_MOUSE_MIDDLE_BUTTON_DOWN,
        RI_MOUSE_BUTTON_4_DOWN,
        RI_MOUSE_BUTTON_5_DOWN
    };

    constexpr const char* XINPUT_MODULES[] = { "xinput9_1_0.dll", "xinput1_4.dll", "xinput1_3.dll" };

    using RawInputDataFn = UINT(WINAPI*)(HRAWINPUT, UINT, LPVOID, PUINT, UINT);
    using XInputGetStateFn = DWORD(WINAPI*)(DWORD, XINPUT_STATE*);

    static std::atomic<uint64_t> s_KeyUntil[KEY_COUNT];
    static std::atomic<uint64_t> s_AllKeysUntil;
    static std::atomic<uint64_t> s_MouseUntil[MOUSE_COUNT];
    static std::atomic<uint64_t> s_WheelUntil;
    static std::atomic<uint64_t> s_MoveUntil;
    static std::atomic<uint64_t> s_PadUntil[PAD_BITS];
    static std::atomic<uint64_t> s_StickUntil[STICK_COUNT];

    static std::atomic_bool s_KeySwallowed[KEY_COUNT];
    static std::atomic<uint64_t> s_KeyLastDown[KEY_COUNT];
    static std::atomic<uint32_t> s_PadSwallowed[XUSER_MAX_COUNT];
    static std::atomic<uint32_t> s_PadApplied[XUSER_MAX_COUNT];
    static std::atomic<uint32_t> s_PadBias[XUSER_MAX_COUNT];

    static RawInputDataFn s_RawInputData = nullptr;
    static XInputGetStateFn s_XInputGetState = nullptr;
    static void** s_RawInputSlot = nullptr;
    static void** s_XInputSlot = nullptr;

    static uint64_t Now()
    {
        return GetTickCount64();
    }

    static bool Active(const std::atomic<uint64_t>& until, uint64_t now)
    {
        return now < until.load(std::memory_order_relaxed);
    }

    static void Hold(std::atomic<uint64_t>& until)
    {
        until.store(Now() + HOLD_MS, std::memory_order_relaxed);
    }

    static bool KeyActive(int key, uint64_t now)
    {
        return key > 0 && key < KEY_COUNT && Active(s_KeyUntil[key], now);
    }

    static bool KeyBlocked(int key, uint64_t now)
    {
        if (Active(s_AllKeysUntil, now) || KeyActive(key, now))
            return true;

        switch (key)
        {
        case VK_SHIFT:
            return KeyActive(VK_LSHIFT, now) || KeyActive(VK_RSHIFT, now);
        case VK_CONTROL:
            return KeyActive(VK_LCONTROL, now) || KeyActive(VK_RCONTROL, now);
        case VK_MENU:
            return KeyActive(VK_LMENU, now) || KeyActive(VK_RMENU, now);
        }
        return false;
    }

    static void FilterKeyboard(RAWKEYBOARD& keyboard, uint64_t now)
    {
        const int key = keyboard.VKey;
        if (key <= 0 || key >= KEY_COUNT - 1)
            return;

        if ((keyboard.Flags & RI_KEY_BREAK) != 0)
        {
            s_KeySwallowed[key] = false;
            return;
        }

        const uint64_t last = s_KeyLastDown[key].exchange(now);
        const bool held = s_KeySwallowed[key] && now - last < REPEAT_GAP_MS;
        if (!held && !KeyBlocked(key, now))
        {
            s_KeySwallowed[key] = false;
            return;
        }

        s_KeySwallowed[key] = true;
        keyboard.Flags |= RI_KEY_BREAK;
        if (keyboard.Message == WM_KEYDOWN)
            keyboard.Message = WM_KEYUP;
        else if (keyboard.Message == WM_SYSKEYDOWN)
            keyboard.Message = WM_SYSKEYUP;
    }

    static void FilterMouse(RAWMOUSE& mouse, uint64_t now)
    {
        const bool swapped = GetSystemMetrics(SM_SWAPBUTTON) != 0;
        for (int button = 0; button < MOUSE_COUNT; button++)
        {
            const int logical = swapped && button < 2 ? 1 - button : button;
            if ((mouse.usButtonFlags & MOUSE_DOWN[button]) != 0 && Active(s_MouseUntil[logical], now))
                mouse.usButtonFlags &= static_cast<USHORT>(~MOUSE_DOWN[button]);
        }

        if (Active(s_WheelUntil, now) && (mouse.usButtonFlags & WHEEL_FLAGS) != 0)
        {
            mouse.usButtonFlags &= static_cast<USHORT>(~WHEEL_FLAGS);
            mouse.usButtonData = 0;
        }

        if (Active(s_MoveUntil, now) && (mouse.usFlags & MOUSE_MOVE_ABSOLUTE) == 0)
        {
            mouse.lLastX = 0;
            mouse.lLastY = 0;
        }
    }

    static UINT WINAPI RawInputDataHook(HRAWINPUT handle, UINT command, LPVOID data, PUINT size, UINT headerSize)
    {
        const UINT result = s_RawInputData(handle, command, data, size, headerSize);
        if (command != RID_INPUT || !data || result == static_cast<UINT>(-1) || headerSize != sizeof(RAWINPUTHEADER))
            return result;

        auto* input = static_cast<RAWINPUT*>(data);
        const uint64_t now = Now();
        if (input->header.dwType == RIM_TYPEKEYBOARD && result >= sizeof(RAWINPUTHEADER) + sizeof(RAWKEYBOARD))
            FilterKeyboard(input->data.keyboard, now);
        else if (input->header.dwType == RIM_TYPEMOUSE && result >= sizeof(RAWINPUTHEADER) + sizeof(RAWMOUSE))
            FilterMouse(input->data.mouse, now);

        return result;
    }

    static uint32_t HeldButtons(const XINPUT_GAMEPAD& pad)
    {
        uint32_t held = pad.wButtons & ~TRIGGER_BITS;
        if (pad.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
            held |= LEFT_TRIGGER;
        if (pad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
            held |= RIGHT_TRIGGER;
        return held;
    }

    static void FilterGamepad(DWORD index, XINPUT_STATE& state, uint64_t now)
    {
        uint32_t blocked = 0;
        for (int bit = 0; bit < PAD_BITS; bit++)
        {
            if (Active(s_PadUntil[bit], now))
                blocked |= 1u << bit;
        }

        auto& pad = state.Gamepad;
        const uint32_t swallowed = (s_PadSwallowed[index] | blocked) & HeldButtons(pad);
        s_PadSwallowed[index] = swallowed;

        const uint32_t masked = swallowed | (blocked & TRIGGER_BITS);
        pad.wButtons &= static_cast<WORD>(~(masked & ~TRIGGER_BITS));
        if ((masked & LEFT_TRIGGER) != 0)
            pad.bLeftTrigger = 0;
        if ((masked & RIGHT_TRIGGER) != 0)
            pad.bRightTrigger = 0;

        uint32_t sticks = 0;
        if (Active(s_StickUntil[0], now))
        {
            pad.sThumbLX = 0;
            pad.sThumbLY = 0;
            sticks |= 1;
        }
        if (Active(s_StickUntil[1], now))
        {
            pad.sThumbRX = 0;
            pad.sThumbRY = 0;
            sticks |= 2;
        }

        const uint32_t applied = masked | (sticks << 16);
        if (s_PadApplied[index].exchange(applied) != applied)
            s_PadBias[index]++;
        state.dwPacketNumber += s_PadBias[index];
    }

    static DWORD WINAPI XInputGetStateHook(DWORD index, XINPUT_STATE* state)
    {
        const DWORD result = s_XInputGetState(index, state);
        if (result == ERROR_SUCCESS && state && index < XUSER_MAX_COUNT)
            FilterGamepad(index, *state, Now());
        return result;
    }

    // walks the game exe's import table to find where it calls the input function from
    static void** FindImport(const char* moduleName, const char* functionName)
    {
        auto* base = reinterpret_cast<uint8_t*>(GetModuleHandleW(nullptr));
        if (!base)
            return nullptr;

        const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
        if (dos->e_magic != IMAGE_DOS_SIGNATURE)
            return nullptr;

        const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
        const auto& directory = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
        if (directory.VirtualAddress == 0)
            return nullptr;

        const HMODULE target = GetModuleHandleA(moduleName);
        const auto address = target ? reinterpret_cast<ULONGLONG>(GetProcAddress(target, functionName)) : 0;

        for (auto* entry = reinterpret_cast<const IMAGE_IMPORT_DESCRIPTOR*>(base + directory.VirtualAddress); entry->Name != 0; entry++)
        {
            if (_stricmp(reinterpret_cast<const char*>(base + entry->Name), moduleName) != 0)
                continue;

            auto* slots = reinterpret_cast<IMAGE_THUNK_DATA*>(base + entry->FirstThunk);
            const auto* names = entry->OriginalFirstThunk ? reinterpret_cast<const IMAGE_THUNK_DATA*>(base + entry->OriginalFirstThunk) : nullptr;

            for (size_t i = 0; slots[i].u1.Function != 0; i++)
            {
                bool named = false;
                if (names && !IMAGE_SNAP_BY_ORDINAL(names[i].u1.Ordinal))
                {
                    const auto* import = reinterpret_cast<const IMAGE_IMPORT_BY_NAME*>(base + names[i].u1.AddressOfData);
                    named = strcmp(reinterpret_cast<const char*>(import->Name), functionName) == 0;
                }

                if (named || (address != 0 && slots[i].u1.Function == address))
                    return reinterpret_cast<void**>(&slots[i].u1.Function);
            }
        }
        return nullptr;
    }

    static bool Patch(void** slot, void* value)
    {
        MEMORY_BASIC_INFORMATION info = {};
        if (!VirtualQuery(slot, &info, sizeof(info)))
            return false;

        const DWORD executable = PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;
        // import slots are read only, unlock, swap the pointer, lock it back
        DWORD protect = 0;
        if (!VirtualProtect(slot, sizeof(void*), (info.Protect & executable) != 0 ? PAGE_EXECUTE_READWRITE : PAGE_READWRITE, &protect))
            return false;

        InterlockedExchangePointer(slot, value);
        VirtualProtect(slot, sizeof(void*), protect, &protect);
        return true;
    }

    template<typename Function>
    static bool Hook(const char* moduleName, const char* functionName, Function detour, Function& original, void**& slot)
    {
        void** found = FindImport(moduleName, functionName);
        if (!found)
            return false;

        original = reinterpret_cast<Function>(*found);
        if (!Patch(found, reinterpret_cast<void*>(detour)))
        {
            Plugin::LogWarning(std::format("Input blocking: couldn't patch {}!{} (error {})", moduleName, functionName, GetLastError()).c_str());
            return false;
        }

        slot = found;
        return true;
    }

    template<typename Function>
    static void Unhook(Function detour, Function original, void**& slot)
    {
        if (slot && *slot == reinterpret_cast<void*>(detour))
            Patch(slot, reinterpret_cast<void*>(original));
        slot = nullptr;
    }

    void Install()
    {
        // point the game's GetRawInputData + XInputGetState at ours so blocked input gets filtered
        if (!s_RawInputSlot)
            Hook("user32.dll", "GetRawInputData", &RawInputDataHook, s_RawInputData, s_RawInputSlot);

        for (const char* module : XINPUT_MODULES)
        {
            if (s_XInputSlot || Hook(module, "XInputGetState", &XInputGetStateHook, s_XInputGetState, s_XInputSlot))
                break;
        }

        Plugin::LogInfo(std::format("Input blocking: keyboard and mouse {}, gamepad {}",
            s_RawInputSlot ? "ready" : "unavailable", s_XInputSlot ? "ready" : "unavailable").c_str());
    }

    void Uninstall()
    {
        Unhook(&RawInputDataHook, s_RawInputData, s_RawInputSlot);
        Unhook(&XInputGetStateHook, s_XInputGetState, s_XInputSlot);
    }

    void BlockKey(RED4ext::IScriptable*, RED4ext::CStackFrame* frame, void*, int64_t)
    {
        int32_t key = 0;
        RED4ext::GetParameter(frame, &key);
        frame->code++;

        if (key > 0 && key < KEY_COUNT)
            Hold(s_KeyUntil[key]);
    }

    void BlockAllKeys(RED4ext::IScriptable*, RED4ext::CStackFrame* frame, void*, int64_t)
    {
        frame->code++;
        Hold(s_AllKeysUntil);
    }

    void BlockMouseButton(RED4ext::IScriptable*, RED4ext::CStackFrame* frame, void*, int64_t)
    {
        int32_t button = 0;
        RED4ext::GetParameter(frame, &button);
        frame->code++;

        if (button >= 0 && button < MOUSE_COUNT)
            Hold(s_MouseUntil[button]);
    }

    void BlockMouseWheel(RED4ext::IScriptable*, RED4ext::CStackFrame* frame, void*, int64_t)
    {
        frame->code++;
        Hold(s_WheelUntil);
    }

    void BlockMouseMove(RED4ext::IScriptable*, RED4ext::CStackFrame* frame, void*, int64_t)
    {
        frame->code++;
        Hold(s_MoveUntil);
    }

    void BlockGamepadButtons(RED4ext::IScriptable*, RED4ext::CStackFrame* frame, void*, int64_t)
    {
        int32_t buttons = 0;
        RED4ext::GetParameter(frame, &buttons);
        frame->code++;

        for (int bit = 0; bit < PAD_BITS; bit++)
        {
            if ((buttons & (1 << bit)) != 0)
                Hold(s_PadUntil[bit]);
        }
    }

    void BlockGamepadStick(RED4ext::IScriptable*, RED4ext::CStackFrame* frame, void*, int64_t)
    {
        int32_t stick = 0;
        RED4ext::GetParameter(frame, &stick);
        frame->code++;

        if (stick >= 0 && stick < STICK_COUNT)
            Hold(s_StickUntil[stick]);
    }

    void ClearInputBlocks(RED4ext::IScriptable*, RED4ext::CStackFrame* frame, void*, int64_t)
    {
        frame->code++;

        for (auto& until : s_KeyUntil) until = 0;
        for (auto& until : s_MouseUntil) until = 0;
        for (auto& until : s_PadUntil) until = 0;
        for (auto& until : s_StickUntil) until = 0;
        s_AllKeysUntil = 0;
        s_WheelUntil = 0;
        s_MoveUntil = 0;
    }

    void GetInputBlockStatus(RED4ext::IScriptable*, RED4ext::CStackFrame* frame, int32_t* out, int64_t)
    {
        frame->code++;
        if (out)
            *out = (s_RawInputSlot ? KeyboardMouse : 0) | (s_XInputSlot ? Gamepad : 0);
    }
}
