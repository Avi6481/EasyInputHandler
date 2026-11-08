#pragma once
#include "Common.h"

#include <string>

namespace EasyTrainer::Input::Keyboard
{
    bool IsKeyDown(int virtualKey);

    void IsKeyPressed(RED4ext::IScriptable*, RED4ext::CStackFrame*, bool* out, int64_t);
    void IsKeyboardActive(RED4ext::IScriptable*, RED4ext::CStackFrame*, bool* out, int64_t);
    void GetPressedKey(RED4ext::IScriptable*, RED4ext::CStackFrame*, int32_t* out, int64_t);
    void GetPressedKeys(RED4ext::IScriptable*, RED4ext::CStackFrame*, RED4ext::DynArray<int32_t>* out, int64_t);

    std::string KeyName(int virtualKey);
    void GetKeyName(RED4ext::IScriptable*, RED4ext::CStackFrame*, RED4ext::CString* out, int64_t);

    void BeginTextCapture(RED4ext::IScriptable*, RED4ext::CStackFrame*, void*, int64_t);
    void EndTextCapture(RED4ext::IScriptable*, RED4ext::CStackFrame*, void*, int64_t);
    void ConsumeTypedText(RED4ext::IScriptable*, RED4ext::CStackFrame*, RED4ext::CString* out, int64_t);

    void SetCursorVisible(bool enable);
    void ShowCursorFn(RED4ext::IScriptable*, RED4ext::CStackFrame*, bool* out, int64_t);
    void IsCursorVisibleFn(RED4ext::IScriptable*, RED4ext::CStackFrame*, bool* out, int64_t);
}
