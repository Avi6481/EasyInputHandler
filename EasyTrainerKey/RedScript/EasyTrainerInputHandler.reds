public native class EasyInputHandler extends IScriptable {
    public native func IsKeyPressed(vkCode: Int32) -> Bool
    public native func IsKeyboardActive() -> Bool
    public native func GetPressedKey() -> Int32
    public native func GetPressedKeys() -> array<Int32>
    public native func GetKeyName(vkCode: Int32) -> String

    public native func ShowCursor(enable: Bool) -> Bool
    public native func IsCursorVisible() -> Bool

    public native func BeginTextCapture() -> Void
    public native func EndTextCapture() -> Void
    public native func ConsumeTypedText() -> String

    public native func GetMouseX() -> Float
    public native func GetMouseY() -> Float
    public native func IsMouseButtonDown(button: Int32) -> Bool
    public native func ConsumeMouseWheel() -> Float
    public native func LogInputDiagnostics() -> Void

    public native func IsGamepadButtonPressed(buttonCode: Int32) -> Bool
    public native func IsGamepadActive() -> Bool
    public native func IsGamepadConnected() -> Bool
    public native func GetPressedGamepadButton() -> Int32
    public native func GetGamepadType() -> Int32
    public native func GetGamepadButtonName(buttonCode: Int32) -> String

    public native func GetGamepadAxis(axisCode: Int32) -> Int32
    public native func IsLeftStickDirection(direction: String) -> Bool
    public native func IsRightStickDirection(direction: String) -> Bool

    public native func GetLeftTrigger() -> Int32
    public native func GetRightTrigger() -> Int32
    public native func GetLeftTriggerNormalized() -> Float
    public native func GetRightTriggerNormalized() -> Float

    public native func SetGamepadVibration(left: Int32, right: Int32) -> Bool

    public native func BlockKey(vkCode: Int32) -> Void
    public native func BlockAllKeys() -> Void
    public native func BlockMouseButton(button: Int32) -> Void
    public native func BlockMouseWheel() -> Void
    public native func BlockMouseMove() -> Void
    public native func BlockGamepadButtons(buttons: Int32) -> Void
    public native func BlockGamepadStick(stick: Int32) -> Void
    public native func ClearInputBlocks() -> Void
    public native func GetInputBlockStatus() -> Int32
}

public class EasyInputScripts {}
