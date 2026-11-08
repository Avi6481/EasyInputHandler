#include "InputHandler/Messages/Messages.h"
#include "Plugin.h"

#include <RED4ext/GpuApi/DeviceData.hpp>

#include <atomic>
#include <format>

namespace EasyTrainer::Input::Messages
{
    constexpr uint64_t WINDOW_REFRESH_MS = 1000;
    constexpr uint64_t REGISTRATION_REFRESH_MS = 500;
    constexpr USHORT GENERIC_PAGE = 0x01;
    constexpr USHORT MOUSE_USAGE = 0x02;
    constexpr USHORT KEYBOARD_USAGE = 0x06;
    constexpr DWORD MOUSE_RELAX_MASK = RIDEV_NOLEGACY | RIDEV_CAPTUREMOUSE;
    constexpr DWORD KEYBOARD_RELAX_MASK = RIDEV_NOLEGACY | RIDEV_APPKEYS;

    static std::mutex s_Mutex;
    static HHOOK s_Hook = nullptr;
    static DWORD s_HookThread = 0;
    static HWND s_Window = nullptr;
    static uint32_t s_RenderWidth = 0;
    static uint32_t s_RenderHeight = 0;
    static uint64_t s_LastWindowRefresh = 0;
    static uint64_t s_LastRegistrationRefresh = 0;
    static std::atomic_bool s_MouseNoLegacy = false;
    static std::atomic_bool s_KeyboardNoLegacy = false;
    static int32_t s_WheelDelta = 0;
    static bool s_RawButtons[static_cast<int32_t>(PhysicalButton::Count)] = {};
    static std::atomic_bool s_Capturing = false;
    static std::string s_Text;
    static wchar_t s_HighSurrogate = 0;
    static std::vector<RAWINPUTDEVICE> s_RelaxedDevices;

    static std::string Narrow(const wchar_t* text)
    {
        const int size = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
        if (size <= 1)
            return {};

        std::string result(static_cast<size_t>(size - 1), '\0');
        WideCharToMultiByte(CP_UTF8, 0, text, -1, result.data(), size, nullptr, nullptr);
        return result;
    }

    static std::string ModuleName(const void* address)
    {
        HMODULE module = nullptr;
        if (!address || !GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            static_cast<LPCWSTR>(address), &module))
            return "unknown";

        wchar_t path[MAX_PATH] = {};
        GetModuleFileNameW(module, path, MAX_PATH);
        return Narrow(std::filesystem::path(path).filename().c_str());
    }

    static std::string ClassName(HWND window)
    {
        if (!window)
            return "none";

        wchar_t name[256] = {};
        GetClassNameW(window, name, 256);
        return Narrow(name);
    }

    static HWND FindSwapChainWindow(uint32_t& width, uint32_t& height)
    {
        auto* data = RED4ext::GpuApi::GetDeviceData();
        if (!data)
            return nullptr;

        for (auto& handle : data->swapChains.resources)
        {
            if (!handle.IsUsed())
                continue;

            auto& swapChain = handle.instance;
            if (!swapChain.swapChain || !swapChain.windowHandle)
                continue;

            DXGI_SWAP_CHAIN_DESC1 desc = {};
            if (FAILED(swapChain.swapChain->GetDesc1(&desc)))
                continue;

            width = desc.Width;
            height = desc.Height;
            return swapChain.windowHandle;
        }
        return nullptr;
    }

    static BOOL CALLBACK PickLargestWindow(HWND window, LPARAM param)
    {
        DWORD processID = 0;
        GetWindowThreadProcessId(window, &processID);
        if (processID != GetCurrentProcessId() || !IsWindowVisible(window) || GetWindow(window, GW_OWNER))
            return TRUE;

        RECT rect = {};
        GetClientRect(window, &rect);
        const auto area = static_cast<int64_t>(rect.right - rect.left) * (rect.bottom - rect.top);

        auto* best = reinterpret_cast<std::pair<HWND, int64_t>*>(param);
        if (area > best->second)
            *best = { window, area };
        return TRUE;
    }

    static void RefreshWindow()
    {
        const uint64_t now = GetTickCount64();
        if (s_Window && IsWindow(s_Window) && now - s_LastWindowRefresh < WINDOW_REFRESH_MS)
            return;
        s_LastWindowRefresh = now;

        uint32_t width = 0;
        uint32_t height = 0;
        HWND window = FindSwapChainWindow(width, height);

        if (!window)
        {
            std::pair<HWND, int64_t> best = { nullptr, 0 };
            EnumWindows(PickLargestWindow, reinterpret_cast<LPARAM>(&best));
            window = best.first;
        }

        s_Window = window;
        s_RenderWidth = width;
        s_RenderHeight = height;
    }

    static std::vector<RAWINPUTDEVICE> RegisteredDevices()
    {
        UINT count = 0;
        GetRegisteredRawInputDevices(nullptr, &count, sizeof(RAWINPUTDEVICE));
        if (count == 0)
            return {};

        std::vector<RAWINPUTDEVICE> devices(count);
        const UINT read = GetRegisteredRawInputDevices(devices.data(), &count, sizeof(RAWINPUTDEVICE));
        if (read == static_cast<UINT>(-1))
            return {};

        devices.resize(read);
        return devices;
    }

    static bool HasNoLegacy(const RAWINPUTDEVICE& device)
    {
        return (device.dwFlags & RIDEV_NOLEGACY) == RIDEV_NOLEGACY;
    }

    static void RefreshRegistrations(bool force)
    {
        const uint64_t now = GetTickCount64();
        if (!force && now - s_LastRegistrationRefresh < REGISTRATION_REFRESH_MS)
            return;
        s_LastRegistrationRefresh = now;

        bool mouse = false;
        bool keyboard = false;
        for (const auto& device : RegisteredDevices())
        {
            if (device.usUsagePage != GENERIC_PAGE || !HasNoLegacy(device))
                continue;

            if (device.usUsage == MOUSE_USAGE)
                mouse = true;
            else if (device.usUsage == KEYBOARD_USAGE)
                keyboard = true;
        }

        if (!mouse && s_MouseNoLegacy)
        {
            for (auto& down : s_RawButtons)
                down = false;
        }

        s_MouseNoLegacy = mouse;
        s_KeyboardNoLegacy = keyboard;
    }

    static void AppendCodepoint(uint32_t codepoint)
    {
        if (codepoint < 0x80)
        {
            s_Text.push_back(static_cast<char>(codepoint));
        }
        else if (codepoint < 0x800)
        {
            s_Text.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
            s_Text.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        }
        else if (codepoint < 0x10000)
        {
            s_Text.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
            s_Text.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
            s_Text.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        }
        else
        {
            s_Text.push_back(static_cast<char>(0xF0 | (codepoint >> 18)));
            s_Text.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
            s_Text.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
            s_Text.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        }
    }

    static void AppendChar(wchar_t character)
    {
        switch (character)
        {
        case 0x08:
            s_Text.push_back('\b');
            return;
        case 0x0D:
        case 0x0A:
            s_Text.push_back('\n');
            return;
        case 0x1B:
            s_Text.push_back('\x1b');
            return;
        }

        if (character < 0x20 || character == 0x7F)
            return;

        if (character >= 0xD800 && character <= 0xDBFF)
        {
            s_HighSurrogate = character;
            return;
        }

        if (character >= 0xDC00 && character <= 0xDFFF)
        {
            if (s_HighSurrogate)
                AppendCodepoint(0x10000 + ((static_cast<uint32_t>(s_HighSurrogate) - 0xD800) << 10) + (static_cast<uint32_t>(character) - 0xDC00));
            s_HighSurrogate = 0;
            return;
        }

        s_HighSurrogate = 0;
        AppendCodepoint(character);
    }

    static void TranslateRawKey(const RAWKEYBOARD& keyboard)
    {
        if ((keyboard.Flags & RI_KEY_BREAK) != 0)
            return;

        switch (keyboard.VKey)
        {
        case VK_BACK:
            AppendChar(0x08);
            return;
        case VK_RETURN:
            AppendChar(0x0D);
            return;
        case VK_ESCAPE:
            AppendChar(0x1B);
            return;
        }

        BYTE state[256] = {};
        for (const int key : { VK_SHIFT, VK_LSHIFT, VK_RSHIFT, VK_CONTROL, VK_LCONTROL, VK_RCONTROL, VK_MENU, VK_LMENU, VK_RMENU })
        {
            if ((GetAsyncKeyState(key) & 0x8000) != 0)
                state[key] = 0x80;
        }
        if ((GetKeyState(VK_CAPITAL) & 0x0001) != 0)
            state[VK_CAPITAL] = 0x01;

        wchar_t buffer[8] = {};
        const int count = ToUnicodeEx(keyboard.VKey, keyboard.MakeCode, state, buffer, 8, 0x4, GetKeyboardLayout(s_HookThread));
        for (int i = 0; i < count; i++)
            AppendChar(buffer[i]);
    }

    static void TrackRawMouse(const RAWMOUSE& mouse)
    {
        const USHORT flags = mouse.usButtonFlags;
        auto set = [](PhysicalButton button, bool down) { s_RawButtons[static_cast<int32_t>(button)] = down; };

        if (flags & RI_MOUSE_LEFT_BUTTON_DOWN) set(PhysicalButton::Left, true);
        if (flags & RI_MOUSE_LEFT_BUTTON_UP) set(PhysicalButton::Left, false);
        if (flags & RI_MOUSE_RIGHT_BUTTON_DOWN) set(PhysicalButton::Right, true);
        if (flags & RI_MOUSE_RIGHT_BUTTON_UP) set(PhysicalButton::Right, false);
        if (flags & RI_MOUSE_MIDDLE_BUTTON_DOWN) set(PhysicalButton::Middle, true);
        if (flags & RI_MOUSE_MIDDLE_BUTTON_UP) set(PhysicalButton::Middle, false);

        if ((flags & RI_MOUSE_WHEEL) != 0 && s_MouseNoLegacy)
            s_WheelDelta += static_cast<SHORT>(mouse.usButtonData);
    }

    static bool WantsRawMouse()
    {
        return s_MouseNoLegacy;
    }

    static bool WantsRawKeyboard()
    {
        return s_Capturing && s_KeyboardNoLegacy;
    }

    static void HandleRawInput(HRAWINPUT handle)
    {
        alignas(8) BYTE buffer[sizeof(RAWINPUT)] = {};
        UINT size = sizeof(buffer);
        if (GetRawInputData(handle, RID_INPUT, buffer, &size, sizeof(RAWINPUTHEADER)) == static_cast<UINT>(-1))
            return;

        const auto* input = reinterpret_cast<const RAWINPUT*>(buffer);
        std::lock_guard lock(s_Mutex);
        if (input->header.dwType == RIM_TYPEMOUSE && WantsRawMouse())
            TrackRawMouse(input->data.mouse);
        else if (input->header.dwType == RIM_TYPEKEYBOARD && WantsRawKeyboard())
            TranslateRawKey(input->data.keyboard);
    }

    static LRESULT CALLBACK GetMessageProc(int code, WPARAM wParam, LPARAM lParam)
    {
        if (code == HC_ACTION && wParam == PM_REMOVE)
        {
            const auto* message = reinterpret_cast<const MSG*>(lParam);
            switch (message->message)
            {
            case WM_MOUSEWHEEL:
                if (!s_MouseNoLegacy)
                {
                    std::lock_guard lock(s_Mutex);
                    s_WheelDelta += GET_WHEEL_DELTA_WPARAM(message->wParam);
                }
                break;
            case WM_CHAR:
                if (s_Capturing && !s_KeyboardNoLegacy)
                {
                    std::lock_guard lock(s_Mutex);
                    AppendChar(static_cast<wchar_t>(message->wParam));
                }
                break;
            case WM_INPUT:
                if (WantsRawMouse() || WantsRawKeyboard())
                    HandleRawInput(reinterpret_cast<HRAWINPUT>(message->lParam));
                break;
            }
        }

        return CallNextHookEx(nullptr, code, wParam, lParam);
    }

    static std::string DescribeFlags(DWORD flags, USHORT usage)
    {
        std::string text;
        auto add = [&text](const char* name) { text += text.empty() ? name : std::string("|") + name; };

        if (HasNoLegacy({ 0, 0, flags, nullptr })) add("NOLEGACY");
        else if (flags & RIDEV_PAGEONLY) add("PAGEONLY");
        if (flags & RIDEV_INPUTSINK) add("INPUTSINK");
        if (flags & RIDEV_EXINPUTSINK) add("EXINPUTSINK");
        if (flags & RIDEV_DEVNOTIFY) add("DEVNOTIFY");
        if (flags & 0x200) add(usage == MOUSE_USAGE ? "CAPTUREMOUSE" : "NOHOTKEYS");
        if (flags & RIDEV_APPKEYS) add("APPKEYS");
        return text.empty() ? "none" : text;
    }

    static void LogLocked(const char* reason)
    {
        RefreshWindow();
        RefreshRegistrations(true);

        Plugin::LogInfo(std::format("Input diagnostics ({})", reason).c_str());

        const DWORD windowThread = s_Window ? GetWindowThreadProcessId(s_Window, nullptr) : 0;
        Plugin::LogInfo(std::format("  Game window {:#x} class '{}' thread {} | caller thread {} | hook thread {} | foreground {}",
            reinterpret_cast<uintptr_t>(s_Window), ClassName(s_Window), windowThread, GetCurrentThreadId(), s_HookThread,
            GetForegroundWindow() == s_Window ? "yes" : "no").c_str());

        if (s_Window)
        {
            const auto procedure = reinterpret_cast<const void*>(GetWindowLongPtrW(s_Window, GWLP_WNDPROC));
            Plugin::LogInfo(std::format("  WndProc {:#x} owned by {}", reinterpret_cast<uintptr_t>(procedure), ModuleName(procedure)).c_str());

            RECT client = {};
            GetClientRect(s_Window, &client);
            Plugin::LogInfo(std::format("  Client {}x{} | swap chain {}x{}", client.right, client.bottom, s_RenderWidth, s_RenderHeight).c_str());
        }

        for (const auto& device : RegisteredDevices())
        {
            const DWORD targetThread = device.hwndTarget ? GetWindowThreadProcessId(device.hwndTarget, nullptr) : 0;
            Plugin::LogInfo(std::format("  Raw input page {:#x} usage {:#x} flags {:#x} [{}] target {:#x} '{}' thread {}",
                device.usUsagePage, device.usUsage, device.dwFlags, DescribeFlags(device.dwFlags, device.usUsage),
                reinterpret_cast<uintptr_t>(device.hwndTarget), ClassName(device.hwndTarget), targetThread).c_str());
        }

        RECT clip = {};
        GetClipCursor(&clip);
        CURSORINFO cursor = { sizeof(CURSORINFO) };
        GetCursorInfo(&cursor);
        Plugin::LogInfo(std::format("  Capture {:#x} | clip ({}, {})-({}, {}) | cursor {} | swapped buttons {}",
            reinterpret_cast<uintptr_t>(GetCapture()), clip.left, clip.top, clip.right, clip.bottom,
            (cursor.flags & CURSOR_SHOWING) ? "showing" : "hidden", GetSystemMetrics(SM_SWAPBUTTON) ? "yes" : "no").c_str());

        Plugin::LogInfo(std::format("  Legacy mouse messages {} | legacy keyboard messages {} | SDL3 loaded {}",
            s_MouseNoLegacy ? "BLOCKED (RIDEV_NOLEGACY)" : "allowed", s_KeyboardNoLegacy ? "BLOCKED (RIDEV_NOLEGACY)" : "allowed",
            GetModuleHandleW(L"SDL3.dll") ? "yes" : "no").c_str());
    }

    bool EnsureInstalled()
    {
        std::lock_guard lock(s_Mutex);
        RefreshWindow();
        RefreshRegistrations(false);

        if (s_Hook)
            return true;
        if (!s_Window)
            return false;

        const DWORD thread = GetWindowThreadProcessId(s_Window, nullptr);
        HMODULE module = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&GetMessageProc), &module);

        // hook the game window's message queue for wheel, typed text and raw input
        s_Hook = SetWindowsHookExW(WH_GETMESSAGE, GetMessageProc, module, thread);
        if (!s_Hook)
        {
            Plugin::LogError(std::format("SetWindowsHookEx failed on thread {} (error {})", thread, GetLastError()).c_str());
            return false;
        }

        s_HookThread = thread;
        LogLocked("message hook installed");
        return true;
    }

    void Shutdown()
    {
        SetLegacyInputRelaxed(false);

        std::lock_guard lock(s_Mutex);
        if (s_Hook)
            UnhookWindowsHookEx(s_Hook);
        s_Hook = nullptr;
    }

    HWND GetWindowInfo(uint32_t& width, uint32_t& height)
    {
        EnsureInstalled();

        std::lock_guard lock(s_Mutex);
        width = s_RenderWidth;
        height = s_RenderHeight;
        return s_Window;
    }

    float ConsumeWheel()
    {
        EnsureInstalled();

        std::lock_guard lock(s_Mutex);
        const float notches = static_cast<float>(s_WheelDelta) / WHEEL_DELTA;
        s_WheelDelta = 0;
        return notches;
    }

    bool IsRawButtonDown(PhysicalButton button)
    {
        EnsureInstalled();

        std::lock_guard lock(s_Mutex);
        return s_RawButtons[static_cast<int32_t>(button)];
    }

    void ClearRawButtons()
    {
        std::lock_guard lock(s_Mutex);
        for (auto& down : s_RawButtons)
            down = false;
    }

    void SetTextCapture(bool enable)
    {
        EnsureInstalled();

        std::lock_guard lock(s_Mutex);
        s_Capturing = enable;
        s_Text.clear();
        s_HighSurrogate = 0;
    }

    std::string ConsumeText()
    {
        EnsureInstalled();

        std::lock_guard lock(s_Mutex);
        std::string text;
        text.swap(s_Text);
        return text;
    }

    void SetLegacyInputRelaxed(bool relax)
    {
        std::lock_guard lock(s_Mutex);

        if (!relax)
        {
            if (s_RelaxedDevices.empty())
                return;

            if (!RegisterRawInputDevices(s_RelaxedDevices.data(), static_cast<UINT>(s_RelaxedDevices.size()), sizeof(RAWINPUTDEVICE)))
                Plugin::LogWarning(std::format("Restoring the game's raw input registration failed (error {})", GetLastError()).c_str());
            else
                Plugin::LogInfo("Restored the game's RIDEV_NOLEGACY raw input registration");

            s_RelaxedDevices.clear();
            RefreshRegistrations(true);
            return;
        }

        if (!s_RelaxedDevices.empty())
            return;

        std::vector<RAWINPUTDEVICE> relaxed;
        for (const auto& device : RegisteredDevices())
        {
            if (device.usUsagePage != GENERIC_PAGE || !HasNoLegacy(device))
                continue;
            if (device.usUsage != MOUSE_USAGE && device.usUsage != KEYBOARD_USAGE)
                continue;

            s_RelaxedDevices.push_back(device);

            auto changed = device;
            changed.dwFlags &= ~(device.usUsage == MOUSE_USAGE ? MOUSE_RELAX_MASK : KEYBOARD_RELAX_MASK);
            relaxed.push_back(changed);
        }

        if (relaxed.empty())
            return;

        if (!RegisterRawInputDevices(relaxed.data(), static_cast<UINT>(relaxed.size()), sizeof(RAWINPUTDEVICE)))
        {
            Plugin::LogWarning(std::format("Lifting RIDEV_NOLEGACY failed (error {}), CET/ImGui will keep missing mouse and keyboard messages", GetLastError()).c_str());
            s_RelaxedDevices.clear();
            return;
        }

        Plugin::LogInfo(std::format("Lifted RIDEV_NOLEGACY on {} raw input device(s) while the cursor is shown", relaxed.size()).c_str());
        RefreshRegistrations(true);
    }

    void LogDiagnostics(const char* reason)
    {
        std::lock_guard lock(s_Mutex);
        LogLocked(reason);
    }
}
