#include "Plugin.h"

namespace EasyTrainer::Plugin
{
    static RED4ext::v1::PluginHandle s_Handle = nullptr;
    static const RED4ext::v1::Sdk* s_Sdk = nullptr;

    void Initialize(RED4ext::v1::PluginHandle handle, const RED4ext::v1::Sdk* sdk)
    {
        s_Handle = handle;
        s_Sdk = sdk;
    }

    std::filesystem::path GetDirectory()
    {
        HMODULE module = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&GetDirectory), &module);

        wchar_t path[MAX_PATH] = {};
        GetModuleFileNameW(module, path, MAX_PATH);
        return std::filesystem::path(path).parent_path();
    }

    void LogInfo(const char* message)
    {
        if (s_Sdk && s_Sdk->logger)
            s_Sdk->logger->Info(s_Handle, message);
    }

    void LogWarning(const char* message)
    {
        if (s_Sdk && s_Sdk->logger)
            s_Sdk->logger->Warn(s_Handle, message);
    }

    void LogError(const char* message)
    {
        if (s_Sdk && s_Sdk->logger)
            s_Sdk->logger->Error(s_Handle, message);
    }
}
