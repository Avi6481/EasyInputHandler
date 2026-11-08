#pragma once
#include "Common.h"

namespace EasyTrainer::Input::Gamepad
{
    void IsGamepadButtonPressed(RED4ext::IScriptable*, RED4ext::CStackFrame*, bool* out, int64_t);
    void IsGamepadActive(RED4ext::IScriptable*, RED4ext::CStackFrame*, bool* out, int64_t);
    void IsGamepadConnected(RED4ext::IScriptable*, RED4ext::CStackFrame*, bool* out, int64_t);
    void GetPressedGamepadButton(RED4ext::IScriptable*, RED4ext::CStackFrame*, int32_t* out, int64_t);
    void GetGamepadType(RED4ext::IScriptable*, RED4ext::CStackFrame*, int32_t* out, int64_t);
    void GetGamepadButtonName(RED4ext::IScriptable*, RED4ext::CStackFrame*, RED4ext::CString* out, int64_t);

    void GetGamepadAxis(RED4ext::IScriptable*, RED4ext::CStackFrame*, int32_t* out, int64_t);
    void IsLeftStickDirection(RED4ext::IScriptable*, RED4ext::CStackFrame*, bool* out, int64_t);
    void IsRightStickDirection(RED4ext::IScriptable*, RED4ext::CStackFrame*, bool* out, int64_t);

    void GetLeftTrigger(RED4ext::IScriptable*, RED4ext::CStackFrame*, int32_t* out, int64_t);
    void GetRightTrigger(RED4ext::IScriptable*, RED4ext::CStackFrame*, int32_t* out, int64_t);
    void GetLeftTriggerNormalized(RED4ext::IScriptable*, RED4ext::CStackFrame*, float* out, int64_t);
    void GetRightTriggerNormalized(RED4ext::IScriptable*, RED4ext::CStackFrame*, float* out, int64_t);

    void SetGamepadVibration(RED4ext::IScriptable*, RED4ext::CStackFrame*, bool* out, int64_t);
}
