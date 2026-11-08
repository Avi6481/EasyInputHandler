#include "InputHandler/Keyboard/Keyboard.h"
#include "InputHandler/Messages/Messages.h"

namespace EasyTrainer::Input::Keyboard
{
    static bool s_CursorEnabled = false;

    // engine force cursor - same reloc CET uses for its overlay, needs engine->unkD0
    static RED4ext::UniversalRelocFunc<void (*)(RED4ext::CBaseEngine::UnkD0*, RED4ext::CName, bool)> s_ForceCursor(2130646213UL);

    void SetCursorVisible(bool enable)
    {
        if (s_CursorEnabled == enable)
            return;

        auto* engine = RED4ext::CGameEngine::Get();
        if (!engine || !engine->unkD0)
            return;

        s_ForceCursor(engine->unkD0, RED4ext::CName("EasyTrainer"), enable);
        ShowCursor(enable ? TRUE : FALSE);
        s_CursorEnabled = enable;

        Messages::EnsureInstalled();
        Messages::SetLegacyInputRelaxed(enable);
    }

    void ShowCursorFn(RED4ext::IScriptable*, RED4ext::CStackFrame* frame, bool* out, int64_t)
    {
        bool enable = false;
        RED4ext::GetParameter(frame, &enable);
        frame->code++;

        SetCursorVisible(enable);
        if (out) *out = s_CursorEnabled;
    }

    void IsCursorVisibleFn(RED4ext::IScriptable*, RED4ext::CStackFrame* frame, bool* out, int64_t)
    {
        frame->code++;
        if (out) *out = s_CursorEnabled;
    }
}
