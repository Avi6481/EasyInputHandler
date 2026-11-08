# EasyInputHandler notes

The full API, every code, and how input blocking works.

## Gamepad codes

Button codes keep the XInput flag values, so existing bindings keep working. PlayStation buttons map by position: Cross = A, Circle = B, Square = X, Triangle = Y.

| Button | Code | Button | Code |
| --- | --- | --- | --- |
| D-pad Up | `0x0001` | Left Trigger | `0x0400` |
| D-pad Down | `0x0002` | Right Trigger | `0x0800` |
| D-pad Left | `0x0004` | South (A / Cross) | `0x1000` |
| D-pad Right | `0x0008` | East (B / Circle) | `0x2000` |
| Start / Options | `0x0010` | West (X / Square) | `0x4000` |
| Back / Share | `0x0020` | North (Y / Triangle) | `0x8000` |
| Left Stick | `0x0040` | Guide / PS | `0x10000` |
| Right Stick | `0x0080` | Touchpad | `0x20000` |
| Left Shoulder | `0x0100` | Misc / Mic | `0x40000` |
| Right Shoulder | `0x0200` | Right Paddle 1 (Elite P1) | `0x80000` |
| Left Paddle 1 (Elite P3) | `0x100000` | Right Paddle 2 (Elite P2) | `0x200000` |
| Left Paddle 2 (Elite P4) | `0x400000` | Misc 2 | `0x800000` |
| Misc 3 | `0x1000000` | Misc 4 | `0x2000000` |
| Misc 5 | `0x4000000` | Misc 6 | `0x8000000` |

That covers every button SDL3 reports. `GetGamepadButtonName(code)` returns the label for the connected controller, for example `0x1000` gives "A" on Xbox, "Cross" on PlayStation and "B" on Switch.

Axes for `GetGamepadAxis`: `0` left X, `1` left Y (up is positive), `2` right X, `3` right Y, `4` left trigger (0-255), `5` right trigger (0-255).

`GetGamepadType` returns `0` none, `1` Xbox, `2` PlayStation 4, `3` PlayStation 5, `4` Switch, `5` other.

## Keyboard, mouse and typing

- **Keys:** `IsKeyPressed` accepts any Windows virtual key from `0x01` to `0xFE`, including Insert `0x2D`, Home `0x24`, Delete `0x2E`, End `0x23`, Page Up/Down `0x21`/`0x22`, F1-F24 `0x70`-`0x87`, the numpad, media and browser keys, and left/right Shift/Ctrl/Alt. `GetPressedKeys()` returns every key held down. `GetKeyName(vk)` returns a display name ("Insert", "Numpad 5", "F13", "Left Ctrl"). Punctuation keys take their name from the current keyboard layout.
- **Mouse:** `GetMouseX/Y` read the cursor in the game window's client area, scaled to the swap-chain size (the same space as CET `GetDisplayResolution()`). `IsMouseButtonDown(0|1|2)` is left, right and middle, and respects swapped buttons. `ConsumeMouseWheel()` returns the notches since the last call, positive up.
- **Typing:** `BeginTextCapture()` starts a buffer. `ConsumeTypedText()` returns the UTF-8 text typed since the last call and clears it; Backspace, Enter and Escape arrive as `\b`, `\n` and `\x1b`. `EndTextCapture()` stops and clears it.

None of this goes through ImGui or CET. A `WH_GETMESSAGE` hook on the game window's thread sees the posted `WM_MOUSEWHEEL`, `WM_CHAR` and `WM_INPUT` messages without replacing anyone's WndProc. If the game registered raw mouse or keyboard input with `RIDEV_NOLEGACY`, the wheel, buttons and text come from `WM_INPUT` instead (keys are translated with `ToUnicodeEx`).

### Why the trainer's ImGui window gets no clicks or typing

`LogInputDiagnostics()` writes the following to `red4ext/logs/easyinputhandler-*.log`. It also runs when the hook is installed and every time `ShowCursor(true)` is called:
- the game window, its thread, and the module that owns its current WndProc (to check that it is still CET's)
- every raw input registration with its flags and target window, including SDL3's
- cursor capture, clip rect and visibility

What the code can fix: if the game registered mouse or keyboard raw input with `RIDEV_NOLEGACY`, Windows never generates `WM_LBUTTONDOWN`, `WM_MOUSEWHEEL` or `WM_CHAR`, so nothing that reads window messages (CET's ImGui included) sees clicks or text. While the cursor is shown, `ShowCursor(true)` re-registers those devices without `RIDEV_NOLEGACY` (and without `CAPTUREMOUSE`/`APPKEYS`, which require it), logs that it did, and puts the game's own registration back on `ShowCursor(false)`.

What it cannot fix: CET only passes window input to ImGui while its own overlay is open. A trainer window drawn with the overlay closed gets no mouse or keyboard from ImGui no matter what. For that case, use the natives above and hit-test in Lua.

SDL3 is initialized on the game's main thread with `SDL_INIT_GAMEPAD` only (no video). Its raw input registrations cover game controllers only and point at SDL's own hidden window, so they don't touch mouse or keyboard messages. The diagnostics list them so you can confirm that.

None of this has been checked in game yet: read the log after opening the menu once.

## Blocking input from the game

The plugin replaces two entries in the game exe's import table: `GetRawInputData` (user32) and `XInputGetState` (xinput9_1_0). Only the game's own calls go through them. The plugin's message hook, SDL3, CET and `GetAsyncKeyState` read input their own way, so the trainer still sees every key it blocks from the game. No status effects or restrictions are applied to V, and nothing is written to saves.

- **Per-frame blocks:** call `BlockKey(vk)`, `BlockAllKeys()`, `BlockMouseButton(0-4)`, `BlockMouseWheel()`, `BlockMouseMove()`, `BlockGamepadButtons(mask)` or `BlockGamepadStick(0|1)` every frame you want the input kept from the game. Each block lasts 250 ms. If the script stops calling them (CET closed, a Lua error), input comes back by itself. `ClearInputBlocks()` drops every block at once.
- **No stuck or leaked presses:** a blocked key press reaches the game as a key release. Key releases always go through, so a key held before the block started can't get stuck. A key or button pressed while blocked stays hidden until it's physically released, so closing the menu with B doesn't also make V crouch.
- **Keys:** `BlockKey(0xA0)` (Left Shift) also blocks the generic Shift key the game reads, and the same goes for Ctrl and Alt.
- **Mouse:** button `0` is the button the trainer reads as left, so it follows swapped buttons. `BlockMouseMove()` zeroes relative mouse movement, which stops the camera while the cursor is shown.
- **Gamepad:** `mask` uses the XInput codes in the table above (D-pad, Start, Back, sticks, bumpers, A/B/X/Y). `0x0400` and `0x0800` block the left and right trigger.
- **Status:** `GetInputBlockStatus()` returns `1` when keyboard and mouse blocking is ready, `2` for gamepad, `3` for both. The RED4ext log also says which hooks went in.

Legacy `WM_KEYDOWN`/`WM_LBUTTONDOWN` messages aren't touched, because CET's ImGui needs them.

## REDscript class

```reds
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
```

`EasyInputScripts` is empty on purpose. The natives get their parameter lists from this file, so if redscript doesn't compile it, every native that takes an argument breaks. Lua checks for `EasyInputScripts` to know the file loaded.

## Lua example

```lua
local handler = EasyInputHandler.new()

local VK_ENTER = 0x0D
local GP_SOUTH = 0x1000

local function SelectPressed()
    return handler:IsKeyPressed(VK_ENTER) or handler:IsGamepadButtonPressed(GP_SOUTH)
end
```
