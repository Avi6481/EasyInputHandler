#pragma once
#include "Common.h"

#include <string>

namespace EasyTrainer::Input::Messages
{
    enum class PhysicalButton : int32_t
    {
        Left = 0,
        Right = 1,
        Middle = 2,
        Count = 3
    };

    bool EnsureInstalled();
    void Shutdown();

    HWND GetWindowInfo(uint32_t& width, uint32_t& height);

    float ConsumeWheel();
    bool IsRawButtonDown(PhysicalButton button);
    void ClearRawButtons();

    void SetTextCapture(bool enable);
    std::string ConsumeText();

    void SetLegacyInputRelaxed(bool relax);
    void LogDiagnostics(const char* reason);
}
