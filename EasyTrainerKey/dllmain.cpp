#include "Common.h"
#include "Plugin.h"
#include "InputHandler/InputHandler.h"
#include "InputHandler/Block/Block.h"
#include "InputHandler/Gamepad/SdlGamepad.h"
#include "InputHandler/Messages/Messages.h"

RED4EXT_C_EXPORT bool RED4EXT_CALL Main(RED4ext::v1::PluginHandle handle, RED4ext::v1::EMainReason reason, const RED4ext::v1::Sdk* sdk)
{
    switch (reason)
    {
    case RED4ext::v1::EMainReason::Load:
    {
        EasyTrainer::Plugin::Initialize(handle, sdk);

        auto* rtti = RED4ext::CRTTISystem::Get();
        rtti->AddRegisterCallback(EasyTrainer::Input::RegisterTypes);
        rtti->AddPostRegisterCallback(EasyTrainer::Input::PostRegisterTypes);

        EasyTrainer::Input::Block::Install();
        break;
    }
    case RED4ext::v1::EMainReason::Unload:
    {
        EasyTrainer::Input::Block::Uninstall();
        EasyTrainer::Input::Messages::Shutdown();
        EasyTrainer::Input::SdlGamepad::Shutdown();
        break;
    }
    }

    return true;
}

RED4EXT_C_EXPORT void RED4EXT_CALL Query(RED4ext::v1::PluginInfo* info)
{
    info->name = L"EasyInputHandler";
    info->author = L"Avi";
    info->version = RED4EXT_V1_SEMVER(3, 0, 0);
    info->runtime = RED4EXT_V1_RUNTIME_VERSION_LATEST;
    info->sdk = RED4EXT_V1_SDK_VERSION_CURRENT;
}

RED4EXT_C_EXPORT uint32_t RED4EXT_CALL Supports()
{
    return RED4EXT_API_VERSION_1;
}
