#pragma once
#include "Common.h"

namespace EasyTrainer::Input::Block
{
    enum Status : int32_t
    {
        KeyboardMouse = 1,
        Gamepad = 2
    };

    void Install();
    void Uninstall();

    void BlockKey(RED4ext::IScriptable*, RED4ext::CStackFrame*, void*, int64_t);
    void BlockAllKeys(RED4ext::IScriptable*, RED4ext::CStackFrame*, void*, int64_t);
    void BlockMouseButton(RED4ext::IScriptable*, RED4ext::CStackFrame*, void*, int64_t);
    void BlockMouseWheel(RED4ext::IScriptable*, RED4ext::CStackFrame*, void*, int64_t);
    void BlockMouseMove(RED4ext::IScriptable*, RED4ext::CStackFrame*, void*, int64_t);
    void BlockGamepadButtons(RED4ext::IScriptable*, RED4ext::CStackFrame*, void*, int64_t);
    void BlockGamepadStick(RED4ext::IScriptable*, RED4ext::CStackFrame*, void*, int64_t);
    void ClearInputBlocks(RED4ext::IScriptable*, RED4ext::CStackFrame*, void*, int64_t);
    void GetInputBlockStatus(RED4ext::IScriptable*, RED4ext::CStackFrame*, int32_t* out, int64_t);
}
