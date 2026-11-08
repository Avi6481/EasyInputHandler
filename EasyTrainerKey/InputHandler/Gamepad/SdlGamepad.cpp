#include "InputHandler/Gamepad/SdlGamepad.h"
#include "Plugin.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <string>
#include <thread>
#include <utility>

namespace EasyTrainer::Input::SdlGamepad
{
    constexpr uint32_t POLL_INTERVAL_MS = 8;
    constexpr uint64_t SCAN_INTERVAL_MS = 1000;
    constexpr uint64_t EMPTY_SCAN_INTERVAL_MS = 250;
    constexpr int SHUTDOWN_WAIT_STEPS = 50;
    constexpr DWORD SHUTDOWN_WAIT_STEP_MS = 10;
    constexpr int32_t TRIGGER_THRESHOLD = 3855;
    constexpr int32_t ACTIVITY_THRESHOLD = 7849;
    constexpr uint32_t RUMBLE_DURATION_MS = 0xFFFF;

    constexpr std::pair<SDL_GamepadButton, uint32_t> BUTTON_MAP[] = {
        { SDL_GAMEPAD_BUTTON_DPAD_UP, Button::DpadUp },
        { SDL_GAMEPAD_BUTTON_DPAD_DOWN, Button::DpadDown },
        { SDL_GAMEPAD_BUTTON_DPAD_LEFT, Button::DpadLeft },
        { SDL_GAMEPAD_BUTTON_DPAD_RIGHT, Button::DpadRight },
        { SDL_GAMEPAD_BUTTON_START, Button::Start },
        { SDL_GAMEPAD_BUTTON_BACK, Button::Back },
        { SDL_GAMEPAD_BUTTON_LEFT_STICK, Button::LeftStick },
        { SDL_GAMEPAD_BUTTON_RIGHT_STICK, Button::RightStick },
        { SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, Button::LeftShoulder },
        { SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, Button::RightShoulder },
        { SDL_GAMEPAD_BUTTON_SOUTH, Button::South },
        { SDL_GAMEPAD_BUTTON_EAST, Button::East },
        { SDL_GAMEPAD_BUTTON_WEST, Button::West },
        { SDL_GAMEPAD_BUTTON_NORTH, Button::North },
        { SDL_GAMEPAD_BUTTON_GUIDE, Button::Guide },
        { SDL_GAMEPAD_BUTTON_TOUCHPAD, Button::Touchpad },
        { SDL_GAMEPAD_BUTTON_MISC1, Button::Misc },
        { SDL_GAMEPAD_BUTTON_RIGHT_PADDLE1, Button::RightPaddle1 },
        { SDL_GAMEPAD_BUTTON_LEFT_PADDLE1, Button::LeftPaddle1 },
        { SDL_GAMEPAD_BUTTON_RIGHT_PADDLE2, Button::RightPaddle2 },
        { SDL_GAMEPAD_BUTTON_LEFT_PADDLE2, Button::LeftPaddle2 },
        { SDL_GAMEPAD_BUTTON_MISC2, Button::Misc2 },
        { SDL_GAMEPAD_BUTTON_MISC3, Button::Misc3 },
        { SDL_GAMEPAD_BUTTON_MISC4, Button::Misc4 },
        { SDL_GAMEPAD_BUTTON_MISC5, Button::Misc5 },
        { SDL_GAMEPAD_BUTTON_MISC6, Button::Misc6 }
    };

    struct ButtonNames
    {
        uint32_t Flag;
        const char* Xbox;
        const char* PlayStation;
        const char* Switch;
    };

    constexpr ButtonNames BUTTON_NAMES[] = {
        { Button::DpadUp, "D-Pad Up", "D-Pad Up", "D-Pad Up" },
        { Button::DpadDown, "D-Pad Down", "D-Pad Down", "D-Pad Down" },
        { Button::DpadLeft, "D-Pad Left", "D-Pad Left", "D-Pad Left" },
        { Button::DpadRight, "D-Pad Right", "D-Pad Right", "D-Pad Right" },
        { Button::Start, "Menu", "Options", "+" },
        { Button::Back, "View", "Create", "-" },
        { Button::LeftStick, "LS", "L3", "LS" },
        { Button::RightStick, "RS", "R3", "RS" },
        { Button::LeftShoulder, "LB", "L1", "L" },
        { Button::RightShoulder, "RB", "R1", "R" },
        { Button::LeftTrigger, "LT", "L2", "ZL" },
        { Button::RightTrigger, "RT", "R2", "ZR" },
        { Button::South, "A", "Cross", "B" },
        { Button::East, "B", "Circle", "A" },
        { Button::West, "X", "Square", "Y" },
        { Button::North, "Y", "Triangle", "X" },
        { Button::Guide, "Xbox", "PS", "Home" },
        { Button::Touchpad, "Touchpad", "Touchpad", "Touchpad" },
        { Button::Misc, "Share", "Mic", "Capture" },
        { Button::RightPaddle1, "P1", "Right Back", "Right SR" },
        { Button::LeftPaddle1, "P3", "Left Back", "Left SL" },
        { Button::RightPaddle2, "P2", "Right Fn", "Right SL" },
        { Button::LeftPaddle2, "P4", "Left Fn", "Left SR" },
        { Button::Misc2, "Misc 2", "Misc 2", "Misc 2" },
        { Button::Misc3, "Misc 3", "Misc 3", "Misc 3" },
        { Button::Misc4, "Misc 4", "Misc 4", "Misc 4" },
        { Button::Misc5, "Misc 5", "Misc 5", "Misc 5" },
        { Button::Misc6, "Misc 6", "Misc 6", "Misc 6" }
    };

    const char* ButtonName(uint32_t button, ControllerType type)
    {
        for (const auto& names : BUTTON_NAMES)
        {
            if (names.Flag != button)
                continue;

            switch (type)
            {
            case ControllerType::PlayStation4:
            case ControllerType::PlayStation5:
                return names.PlayStation;
            case ControllerType::Switch:
                return names.Switch;
            default:
                return names.Xbox;
            }
        }
        return "";
    }

    struct OpenGamepad
    {
        SDL_JoystickID Id;
        SDL_Gamepad* Handle;
    };

    static std::mutex s_Mutex;
    static State s_State;
    static std::once_flag s_StartFlag;
    static std::thread s_Thread;
    static std::atomic_bool s_Running = false;
    static std::atomic_bool s_Stopped = true;
    static std::atomic<uint32_t> s_Rumble = 0;
    static std::atomic_bool s_RumbleDirty = false;

    static std::vector<OpenGamepad> s_Gamepads;
    static SDL_Gamepad* s_Active = nullptr;
    static uint64_t s_LastScan = 0;

    static bool Initialize()
    {
        const auto directory = Plugin::GetDirectory();
        if (!LoadLibraryExW((directory / L"SDL3.dll").c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH))
        {
            Plugin::LogError("SDL3.dll was not found next to EasyInputHandler.dll, gamepad input is disabled");
            return false;
        }

        SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
        SDL_SetHint(SDL_HINT_JOYSTICK_THREAD, "1");

        if (!SDL_Init(SDL_INIT_GAMEPAD))
        {
            Plugin::LogError((std::string("SDL_Init failed: ") + SDL_GetError()).c_str());
            return false;
        }

        const auto mappings = directory / L"gamecontrollerdb.txt";
        if (std::filesystem::exists(mappings))
            SDL_AddGamepadMappingsFromFile(mappings.string().c_str());

        Plugin::LogInfo("SDL gamepad input initialized");
        return true;
    }

    static bool IsOpen(SDL_JoystickID id)
    {
        return std::any_of(s_Gamepads.begin(), s_Gamepads.end(), [id](const OpenGamepad& pad) { return pad.Id == id; });
    }

    static void CloseDisconnected()
    {
        std::erase_if(s_Gamepads, [](const OpenGamepad& pad)
        {
            if (SDL_GamepadConnected(pad.Handle))
                return false;

            if (s_Active == pad.Handle)
                s_Active = nullptr;
            SDL_CloseGamepad(pad.Handle);
            return true;
        });
    }

    static void Scan(uint64_t now)
    {
        const uint64_t interval = s_Gamepads.empty() ? EMPTY_SCAN_INTERVAL_MS : SCAN_INTERVAL_MS;
        if (now - s_LastScan < interval)
            return;
        s_LastScan = now;

        int count = 0;
        SDL_JoystickID* ids = SDL_GetGamepads(&count);
        if (!ids)
            return;

        for (int i = 0; i < count; i++)
        {
            if (IsOpen(ids[i]))
                continue;

            SDL_Gamepad* handle = SDL_OpenGamepad(ids[i]);
            if (!handle)
                continue;

            s_Gamepads.push_back({ ids[i], handle });
            const char* name = SDL_GetGamepadName(handle);
            Plugin::LogInfo((std::string("Gamepad connected: ") + (name ? name : "Unknown")).c_str());
        }

        SDL_free(ids);
    }

    static uint32_t ReadButtons(SDL_Gamepad* gamepad)
    {
        uint32_t buttons = 0;
        for (const auto& [button, flag] : BUTTON_MAP)
        {
            if (SDL_GetGamepadButton(gamepad, button))
                buttons |= flag;
        }

        if (SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER) > TRIGGER_THRESHOLD)
            buttons |= Button::LeftTrigger;
        if (SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) > TRIGGER_THRESHOLD)
            buttons |= Button::RightTrigger;

        return buttons;
    }

    static bool HasActivity(SDL_Gamepad* gamepad, uint32_t buttons)
    {
        if (buttons != 0)
            return true;

        for (const auto axis : { SDL_GAMEPAD_AXIS_LEFTX, SDL_GAMEPAD_AXIS_LEFTY, SDL_GAMEPAD_AXIS_RIGHTX, SDL_GAMEPAD_AXIS_RIGHTY })
        {
            if (std::abs(static_cast<int32_t>(SDL_GetGamepadAxis(gamepad, axis))) > ACTIVITY_THRESHOLD)
                return true;
        }
        return false;
    }

    static ControllerType ToControllerType(SDL_GamepadType type)
    {
        switch (type)
        {
        case SDL_GAMEPAD_TYPE_XBOX360:
        case SDL_GAMEPAD_TYPE_XBOXONE:
            return ControllerType::Xbox;
        case SDL_GAMEPAD_TYPE_PS3:
        case SDL_GAMEPAD_TYPE_PS4:
            return ControllerType::PlayStation4;
        case SDL_GAMEPAD_TYPE_PS5:
            return ControllerType::PlayStation5;
        case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_PRO:
        case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_LEFT:
        case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_RIGHT:
        case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_PAIR:
            return ControllerType::Switch;
        default:
            return ControllerType::Other;
        }
    }

    static int32_t InvertAxis(int16_t value)
    {
        return std::min(32767, -static_cast<int32_t>(value));
    }

    static int32_t TriggerToByte(int16_t value)
    {
        return std::clamp((static_cast<int32_t>(value) * 255) / 32767, 0, 255);
    }

    static State ReadState(SDL_Gamepad* gamepad, uint32_t buttons)
    {
        State state;
        state.Connected = true;
        state.Buttons = buttons;
        state.Type = ToControllerType(SDL_GetGamepadType(gamepad));
        state.Axes[static_cast<int32_t>(Axis::LeftX)] = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTX);
        state.Axes[static_cast<int32_t>(Axis::LeftY)] = InvertAxis(SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTY));
        state.Axes[static_cast<int32_t>(Axis::RightX)] = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHTX);
        state.Axes[static_cast<int32_t>(Axis::RightY)] = InvertAxis(SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHTY));
        state.Axes[static_cast<int32_t>(Axis::LeftTrigger)] = TriggerToByte(SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER));
        state.Axes[static_cast<int32_t>(Axis::RightTrigger)] = TriggerToByte(SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER));
        return state;
    }

    static void ApplyRumble()
    {
        if (!s_RumbleDirty.exchange(false) || !s_Active)
            return;

        const uint32_t packed = s_Rumble;
        const auto low = static_cast<uint16_t>(packed >> 16);
        const auto high = static_cast<uint16_t>(packed & 0xFFFF);
        const uint32_t duration = (low == 0 && high == 0) ? 0 : RUMBLE_DURATION_MS;
        SDL_RumbleGamepad(s_Active, low, high, duration);
    }

    static void Update()
    {
        SDL_UpdateGamepads();
        CloseDisconnected();
        Scan(SDL_GetTicks());

        uint32_t buttons = 0;
        for (const auto& pad : s_Gamepads)
        {
            const uint32_t padButtons = ReadButtons(pad.Handle);
            buttons |= padButtons;
            if (HasActivity(pad.Handle, padButtons))
                s_Active = pad.Handle;
        }

        if (!s_Active && !s_Gamepads.empty())
            s_Active = s_Gamepads.front().Handle;

        ApplyRumble();

        const State state = s_Active ? ReadState(s_Active, buttons) : State{};
        std::lock_guard lock(s_Mutex);
        s_State = state;
    }

    static void CloseAll()
    {
        for (const auto& pad : s_Gamepads)
            SDL_CloseGamepad(pad.Handle);

        s_Gamepads.clear();
        s_Active = nullptr;
    }

    static void Run()
    {
        if (Initialize())
        {
            while (s_Running)
            {
                Update();
                SDL_Delay(POLL_INTERVAL_MS);
            }

            CloseAll();
            SDL_Quit();
        }

        s_Stopped = true;
    }

    static void Start()
    {
        std::call_once(s_StartFlag, []
        {
            s_Running = true;
            s_Stopped = false;
            s_Thread = std::thread(Run);
        });
    }

    State Poll()
    {
        Start();

        std::lock_guard lock(s_Mutex);
        return s_State;
    }

    bool Rumble(uint16_t low, uint16_t high)
    {
        Start();

        s_Rumble = (static_cast<uint32_t>(low) << 16) | high;
        s_RumbleDirty = true;

        std::lock_guard lock(s_Mutex);
        return s_State.Connected;
    }

    void Shutdown()
    {
        if (!s_Thread.joinable())
            return;

        s_Running = false;
        for (int step = 0; step < SHUTDOWN_WAIT_STEPS && !s_Stopped; step++)
            Sleep(SHUTDOWN_WAIT_STEP_MS);

        s_Thread.detach();
    }
}
