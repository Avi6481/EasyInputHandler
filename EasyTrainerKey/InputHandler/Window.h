#pragma once
#include "Common.h"

namespace EasyTrainer::Input::Window
{
    inline bool IsGameFocused()
    {
        const HWND window = GetForegroundWindow();
        if (!window)
            return false;

        DWORD processID = 0;
        GetWindowThreadProcessId(window, &processID);
        return processID == GetCurrentProcessId();
    }
}
