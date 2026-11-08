#include "InputHandler/InputHandler.h"
#include "InputHandler/Keyboard/Keyboard.h"
#include "InputHandler/Gamepad/Gamepad.h"
#include "InputHandler/Mouse/Mouse.h"
#include "InputHandler/Block/Block.h"

namespace EasyTrainer::Input
{
    static RED4ext::TTypedClass<InputHandler> s_InputHandlerType("EasyInputHandler");

    RED4ext::CClass* InputHandler::GetNativeType()
    {
        return &s_InputHandlerType;
    }

    template<typename Function>
    static void RegisterNative(const char* name, const char* returnType, Function function)
    {
        auto* native = RED4ext::CClassFunction::Create(&s_InputHandlerType, name, name, function);
        native->flags = { .isNative = true };
        if (returnType)
            native->SetReturnType(returnType);
        s_InputHandlerType.RegisterFunction(native);
    }

    void RegisterTypes()
    {
        RED4ext::CNamePool::Add("EasyInputHandler");
        s_InputHandlerType.flags = { .isNative = true };
        RED4ext::CRTTISystem::Get()->RegisterType(&s_InputHandlerType);
    }

    void PostRegisterTypes()
    {
        s_InputHandlerType.parent = RED4ext::CRTTISystem::Get()->GetClass("IScriptable");

        RegisterNative("IsKeyPressed", "Bool", &Keyboard::IsKeyPressed);
        RegisterNative("IsKeyboardActive", "Bool", &Keyboard::IsKeyboardActive);
        RegisterNative("GetPressedKey", "Int32", &Keyboard::GetPressedKey);
        RegisterNative("GetPressedKeys", "array:Int32", &Keyboard::GetPressedKeys);
        RegisterNative("GetKeyName", "String", &Keyboard::GetKeyName);
        RegisterNative("ShowCursor", "Bool", &Keyboard::ShowCursorFn);
        RegisterNative("IsCursorVisible", "Bool", &Keyboard::IsCursorVisibleFn);

        RegisterNative("BeginTextCapture", nullptr, &Keyboard::BeginTextCapture);
        RegisterNative("EndTextCapture", nullptr, &Keyboard::EndTextCapture);
        RegisterNative("ConsumeTypedText", "String", &Keyboard::ConsumeTypedText);

        RegisterNative("GetMouseX", "Float", &Mouse::GetMouseX);
        RegisterNative("GetMouseY", "Float", &Mouse::GetMouseY);
        RegisterNative("IsMouseButtonDown", "Bool", &Mouse::IsMouseButtonDown);
        RegisterNative("ConsumeMouseWheel", "Float", &Mouse::ConsumeMouseWheel);
        RegisterNative("LogInputDiagnostics", nullptr, &Mouse::LogInputDiagnostics);

        RegisterNative("IsGamepadButtonPressed", "Bool", &Gamepad::IsGamepadButtonPressed);
        RegisterNative("IsGamepadActive", "Bool", &Gamepad::IsGamepadActive);
        RegisterNative("IsGamepadConnected", "Bool", &Gamepad::IsGamepadConnected);
        RegisterNative("GetPressedGamepadButton", "Int32", &Gamepad::GetPressedGamepadButton);
        RegisterNative("GetGamepadType", "Int32", &Gamepad::GetGamepadType);
        RegisterNative("GetGamepadButtonName", "String", &Gamepad::GetGamepadButtonName);
        RegisterNative("GetGamepadAxis", "Int32", &Gamepad::GetGamepadAxis);
        RegisterNative("IsLeftStickDirection", "Bool", &Gamepad::IsLeftStickDirection);
        RegisterNative("IsRightStickDirection", "Bool", &Gamepad::IsRightStickDirection);
        RegisterNative("GetLeftTrigger", "Int32", &Gamepad::GetLeftTrigger);
        RegisterNative("GetRightTrigger", "Int32", &Gamepad::GetRightTrigger);
        RegisterNative("GetLeftTriggerNormalized", "Float", &Gamepad::GetLeftTriggerNormalized);
        RegisterNative("GetRightTriggerNormalized", "Float", &Gamepad::GetRightTriggerNormalized);
        RegisterNative("SetGamepadVibration", "Bool", &Gamepad::SetGamepadVibration);

        RegisterNative("BlockKey", nullptr, &Block::BlockKey);
        RegisterNative("BlockAllKeys", nullptr, &Block::BlockAllKeys);
        RegisterNative("BlockMouseButton", nullptr, &Block::BlockMouseButton);
        RegisterNative("BlockMouseWheel", nullptr, &Block::BlockMouseWheel);
        RegisterNative("BlockMouseMove", nullptr, &Block::BlockMouseMove);
        RegisterNative("BlockGamepadButtons", nullptr, &Block::BlockGamepadButtons);
        RegisterNative("BlockGamepadStick", nullptr, &Block::BlockGamepadStick);
        RegisterNative("ClearInputBlocks", nullptr, &Block::ClearInputBlocks);
        RegisterNative("GetInputBlockStatus", "Int32", &Block::GetInputBlockStatus);
    }
}
