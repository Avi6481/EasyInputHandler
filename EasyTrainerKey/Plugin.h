#pragma once
#include "Common.h"

namespace EasyTrainer::Plugin
{
    void Initialize(RED4ext::v1::PluginHandle handle, const RED4ext::v1::Sdk* sdk);

    std::filesystem::path GetDirectory();

    void LogInfo(const char* message);
    void LogWarning(const char* message);
    void LogError(const char* message);
}
