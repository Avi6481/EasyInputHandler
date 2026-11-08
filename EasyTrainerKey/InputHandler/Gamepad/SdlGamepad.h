#pragma once
#include "Common.h"

namespace EasyTrainer::Input::SdlGamepad
{
    enum class ControllerType : int32_t
    {
        None = 0,
        Xbox = 1,
        PlayStation4 = 2,
        PlayStation5 = 3,
        Switch = 4,
        Other = 5
    };

    namespace Button
    {
        constexpr uint32_t DpadUp = 0x0001;
        constexpr uint32_t DpadDown = 0x0002;
        constexpr uint32_t DpadLeft = 0x0004;
        constexpr uint32_t DpadRight = 0x0008;
        constexpr uint32_t Start = 0x0010;
        constexpr uint32_t Back = 0x0020;
        constexpr uint32_t LeftStick = 0x0040;
        constexpr uint32_t RightStick = 0x0080;
        constexpr uint32_t LeftShoulder = 0x0100;
        constexpr uint32_t RightShoulder = 0x0200;
        constexpr uint32_t LeftTrigger = 0x0400;
        constexpr uint32_t RightTrigger = 0x0800;
        constexpr uint32_t South = 0x1000;
        constexpr uint32_t East = 0x2000;
        constexpr uint32_t West = 0x4000;
        constexpr uint32_t North = 0x8000;
        constexpr uint32_t Guide = 0x10000;
        constexpr uint32_t Touchpad = 0x20000;
        constexpr uint32_t Misc = 0x40000;
        constexpr uint32_t RightPaddle1 = 0x80000;
        constexpr uint32_t LeftPaddle1 = 0x100000;
        constexpr uint32_t RightPaddle2 = 0x200000;
        constexpr uint32_t LeftPaddle2 = 0x400000;
        constexpr uint32_t Misc2 = 0x800000;
        constexpr uint32_t Misc3 = 0x1000000;
        constexpr uint32_t Misc4 = 0x2000000;
        constexpr uint32_t Misc5 = 0x4000000;
        constexpr uint32_t Misc6 = 0x8000000;
    }

    enum class Axis : int32_t
    {
        LeftX = 0,
        LeftY = 1,
        RightX = 2,
        RightY = 3,
        LeftTrigger = 4,
        RightTrigger = 5,
        Count = 6
    };

    struct State
    {
        bool Connected = false;
        uint32_t Buttons = 0;
        int32_t Axes[static_cast<int32_t>(Axis::Count)] = {};
        ControllerType Type = ControllerType::None;
    };

    const char* ButtonName(uint32_t button, ControllerType type);

    State Poll();
    bool Rumble(uint16_t low, uint16_t high);
    void Shutdown();
}
