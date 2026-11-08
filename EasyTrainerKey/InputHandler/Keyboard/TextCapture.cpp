#include "InputHandler/Keyboard/Keyboard.h"
#include "InputHandler/Messages/Messages.h"

namespace EasyTrainer::Input::Keyboard
{
    void BeginTextCapture(RED4ext::IScriptable*, RED4ext::CStackFrame* frame, void*, int64_t)
    {
        frame->code++;
        Messages::SetTextCapture(true);
    }

    void EndTextCapture(RED4ext::IScriptable*, RED4ext::CStackFrame* frame, void*, int64_t)
    {
        frame->code++;
        Messages::SetTextCapture(false);
    }

    void ConsumeTypedText(RED4ext::IScriptable*, RED4ext::CStackFrame* frame, RED4ext::CString* out, int64_t)
    {
        frame->code++;

        auto text = Messages::ConsumeText();
        if (out) *out = RED4ext::CString(text.c_str());
    }
}
