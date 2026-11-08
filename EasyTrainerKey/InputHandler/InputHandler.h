#pragma once
#include "Common.h"

namespace EasyTrainer::Input
{
    struct InputHandler : RED4ext::IScriptable
    {
        RED4ext::CClass* GetNativeType() override;
    };

    void RegisterTypes();
    void PostRegisterTypes();
}
