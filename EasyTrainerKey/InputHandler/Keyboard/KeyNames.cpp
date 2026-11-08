#include "InputHandler/Keyboard/Keyboard.h"

#include <format>
#include <unordered_map>

namespace EasyTrainer::Input::Keyboard
{
    static const std::unordered_map<int, const char*> s_KeyNames = {
        { VK_LBUTTON, "Left Mouse" }, { VK_RBUTTON, "Right Mouse" }, { VK_CANCEL, "Break" }, { VK_MBUTTON, "Middle Mouse" },
        { VK_XBUTTON1, "Mouse 4" }, { VK_XBUTTON2, "Mouse 5" },
        { VK_BACK, "Backspace" }, { VK_TAB, "Tab" }, { VK_CLEAR, "Clear" }, { VK_RETURN, "Enter" },
        { VK_SHIFT, "Shift" }, { VK_CONTROL, "Ctrl" }, { VK_MENU, "Alt" }, { VK_PAUSE, "Pause" }, { VK_CAPITAL, "Caps Lock" },
        { VK_KANA, "Kana" }, { VK_IME_ON, "IME On" }, { VK_JUNJA, "Junja" }, { VK_FINAL, "Final" }, { VK_KANJI, "Kanji" },
        { VK_IME_OFF, "IME Off" }, { VK_ESCAPE, "Escape" }, { VK_CONVERT, "Convert" }, { VK_NONCONVERT, "Non Convert" },
        { VK_ACCEPT, "Accept" }, { VK_MODECHANGE, "Mode Change" },
        { VK_SPACE, "Space" }, { VK_PRIOR, "Page Up" }, { VK_NEXT, "Page Down" }, { VK_END, "End" }, { VK_HOME, "Home" },
        { VK_LEFT, "Left" }, { VK_UP, "Up" }, { VK_RIGHT, "Right" }, { VK_DOWN, "Down" },
        { VK_SELECT, "Select" }, { VK_PRINT, "Print" }, { VK_EXECUTE, "Execute" }, { VK_SNAPSHOT, "Print Screen" },
        { VK_INSERT, "Insert" }, { VK_DELETE, "Delete" }, { VK_HELP, "Help" },
        { VK_LWIN, "Left Windows" }, { VK_RWIN, "Right Windows" }, { VK_APPS, "Menu" }, { VK_SLEEP, "Sleep" },
        { VK_NUMPAD0, "Numpad 0" }, { VK_NUMPAD1, "Numpad 1" }, { VK_NUMPAD2, "Numpad 2" }, { VK_NUMPAD3, "Numpad 3" },
        { VK_NUMPAD4, "Numpad 4" }, { VK_NUMPAD5, "Numpad 5" }, { VK_NUMPAD6, "Numpad 6" }, { VK_NUMPAD7, "Numpad 7" },
        { VK_NUMPAD8, "Numpad 8" }, { VK_NUMPAD9, "Numpad 9" },
        { VK_MULTIPLY, "Numpad *" }, { VK_ADD, "Numpad +" }, { VK_SEPARATOR, "Numpad Separator" }, { VK_SUBTRACT, "Numpad -" },
        { VK_DECIMAL, "Numpad ." }, { VK_DIVIDE, "Numpad /" },
        { VK_NUMLOCK, "Num Lock" }, { VK_SCROLL, "Scroll Lock" },
        { VK_LSHIFT, "Left Shift" }, { VK_RSHIFT, "Right Shift" }, { VK_LCONTROL, "Left Ctrl" }, { VK_RCONTROL, "Right Ctrl" },
        { VK_LMENU, "Left Alt" }, { VK_RMENU, "Right Alt" },
        { VK_BROWSER_BACK, "Browser Back" }, { VK_BROWSER_FORWARD, "Browser Forward" }, { VK_BROWSER_REFRESH, "Browser Refresh" },
        { VK_BROWSER_STOP, "Browser Stop" }, { VK_BROWSER_SEARCH, "Browser Search" }, { VK_BROWSER_FAVORITES, "Browser Favorites" },
        { VK_BROWSER_HOME, "Browser Home" },
        { VK_VOLUME_MUTE, "Volume Mute" }, { VK_VOLUME_DOWN, "Volume Down" }, { VK_VOLUME_UP, "Volume Up" },
        { VK_MEDIA_NEXT_TRACK, "Next Track" }, { VK_MEDIA_PREV_TRACK, "Previous Track" }, { VK_MEDIA_STOP, "Media Stop" },
        { VK_MEDIA_PLAY_PAUSE, "Play/Pause" },
        { VK_LAUNCH_MAIL, "Launch Mail" }, { VK_LAUNCH_MEDIA_SELECT, "Launch Media" }, { VK_LAUNCH_APP1, "Launch App 1" },
        { VK_LAUNCH_APP2, "Launch App 2" },
        { VK_PROCESSKEY, "Process" }, { VK_PACKET, "Packet" }, { VK_ATTN, "Attn" }, { VK_CRSEL, "CrSel" }, { VK_EXSEL, "ExSel" },
        { VK_EREOF, "Erase EOF" }, { VK_PLAY, "Play" }, { VK_ZOOM, "Zoom" }, { VK_PA1, "PA1" }, { VK_OEM_CLEAR, "OEM Clear" },
        { VK_GAMEPAD_A, "Gamepad A" }, { VK_GAMEPAD_B, "Gamepad B" }, { VK_GAMEPAD_X, "Gamepad X" }, { VK_GAMEPAD_Y, "Gamepad Y" },
        { VK_GAMEPAD_RIGHT_SHOULDER, "Gamepad RB" }, { VK_GAMEPAD_LEFT_SHOULDER, "Gamepad LB" },
        { VK_GAMEPAD_LEFT_TRIGGER, "Gamepad LT" }, { VK_GAMEPAD_RIGHT_TRIGGER, "Gamepad RT" },
        { VK_GAMEPAD_DPAD_UP, "Gamepad Up" }, { VK_GAMEPAD_DPAD_DOWN, "Gamepad Down" }, { VK_GAMEPAD_DPAD_LEFT, "Gamepad Left" },
        { VK_GAMEPAD_DPAD_RIGHT, "Gamepad Right" }, { VK_GAMEPAD_MENU, "Gamepad Menu" }, { VK_GAMEPAD_VIEW, "Gamepad View" },
        { VK_GAMEPAD_LEFT_THUMBSTICK_BUTTON, "Gamepad LS" }, { VK_GAMEPAD_RIGHT_THUMBSTICK_BUTTON, "Gamepad RS" }
    };

    static std::string LayoutName(int virtualKey)
    {
        const UINT character = MapVirtualKeyW(static_cast<UINT>(virtualKey), MAPVK_VK_TO_CHAR) & 0x7FFFFFFF;
        if (character > 0x20)
        {
            const wchar_t text[2] = { static_cast<wchar_t>(character), 0 };
            char buffer[8] = {};
            WideCharToMultiByte(CP_UTF8, 0, text, -1, buffer, sizeof(buffer), nullptr, nullptr);
            return buffer;
        }

        const UINT scanCode = MapVirtualKeyW(static_cast<UINT>(virtualKey), MAPVK_VK_TO_VSC);
        if (scanCode == 0)
            return {};

        wchar_t name[64] = {};
        if (GetKeyNameTextW(static_cast<LONG>(scanCode << 16), name, 64) <= 0)
            return {};

        char buffer[128] = {};
        WideCharToMultiByte(CP_UTF8, 0, name, -1, buffer, sizeof(buffer), nullptr, nullptr);
        return buffer;
    }

    std::string KeyName(int virtualKey)
    {
        if (virtualKey >= VK_F1 && virtualKey <= VK_F24)
            return std::format("F{}", virtualKey - VK_F1 + 1);

        if ((virtualKey >= '0' && virtualKey <= '9') || (virtualKey >= 'A' && virtualKey <= 'Z'))
            return std::string(1, static_cast<char>(virtualKey));

        if (auto it = s_KeyNames.find(virtualKey); it != s_KeyNames.end())
            return it->second;

        if (auto name = LayoutName(virtualKey); !name.empty())
            return name;

        return std::format("Key 0x{:02X}", virtualKey);
    }

    void GetKeyName(RED4ext::IScriptable*, RED4ext::CStackFrame* frame, RED4ext::CString* out, int64_t)
    {
        int32_t key = 0;
        RED4ext::GetParameter(frame, &key);
        frame->code++;

        if (out) *out = RED4ext::CString(KeyName(key).c_str());
    }
}
