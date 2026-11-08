#pragma once
#include "Common.h"

namespace EasyTrainer::Input::Mouse
{
    void GetMouseX(RED4ext::IScriptable*, RED4ext::CStackFrame*, float* out, int64_t);
    void GetMouseY(RED4ext::IScriptable*, RED4ext::CStackFrame*, float* out, int64_t);
    void IsMouseButtonDown(RED4ext::IScriptable*, RED4ext::CStackFrame*, bool* out, int64_t);
    void ConsumeMouseWheel(RED4ext::IScriptable*, RED4ext::CStackFrame*, float* out, int64_t);
    void LogInputDiagnostics(RED4ext::IScriptable*, RED4ext::CStackFrame*, void*, int64_t);
}
