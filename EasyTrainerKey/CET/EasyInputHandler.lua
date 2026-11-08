---@class EasyInputHandler : IScriptable
EasyInputHandler = {}

---Check if a virtual key is currently pressed.  
---Pass a Windows virtual key code, e.g. 0x20 for Spacebar, 0x41 for "A".  
---Example: if EasyInputHandler.IsKeyPressed(0x20) then print("Space pressed") end  
---@param vkCode integer Virtual key code  
---@return boolean True if the key is currently held down  
function EasyInputHandler.IsKeyPressed(vkCode) end

---Check if the keyboard is the active input device.  
---@return boolean  
function EasyInputHandler.IsKeyboardActive() end

---Get the last pressed key code.  
---@return integer Virtual key code  
function EasyInputHandler.GetPressedKey() end

---Check if a specific gamepad button is pressed.  
---Pass one of the XInput button flags, e.g. 0x1000 for A, 0x2000 for B.  
---Example: if EasyInputHandler.IsGamepadButtonPressed(0x1000) then print("A pressed") end  
---@param buttonCode integer XInput gamepad button flag  
---@return boolean True if the button is currently held down  
function EasyInputHandler.IsGamepadButtonPressed(buttonCode) end

---Check if a gamepad is currently the active input device.  
---@return boolean  
function EasyInputHandler.IsGamepadActive() end

---Get the last pressed gamepad button code.  
---@return integer Button ID  
function EasyInputHandler.GetPressedGamepadButton() end

---Get raw axis value from the gamepad.  
---Usually in the range -32768..32767 depending on axis.  
---@param axisCode integer Axis ID  
---@return integer  
function EasyInputHandler.GetGamepadAxis(axisCode) end

---Show or hide the mouse cursor.  
---@param enable boolean True = show, false = hide  
---@return boolean New cursor visibility state  
function EasyInputHandler.ShowCursor(enable) end

---Check if the mouse cursor is visible.  
---@return boolean  
function EasyInputHandler.IsCursorVisible() end

---Check if the left stick is tilted in a specific direction.  
---Valid directions: "up", "down", "left", "right",  
---"upleft", "upright", "downleft", "downright"  
---@param direction string  
---@return boolean  
function EasyInputHandler.IsLeftStickDirection(direction) end

---Check if the right stick is tilted in a specific direction.  
---Valid directions: "up", "down", "left", "right",  
---"upleft", "upright", "downleft", "downright"  
---@param direction string  
---@return boolean  
function EasyInputHandler.IsRightStickDirection(direction) end

---Get raw left trigger value.  
---@return integer Range usually 0–65535  
function EasyInputHandler.GetLeftTrigger() end

---Get raw right trigger value.  
---@return integer Range usually 0–65535  
function EasyInputHandler.GetRightTrigger() end

---Get normalized left trigger value (0.0–1.0).  
---@return number  
function EasyInputHandler.GetLeftTriggerNormalized() end

---Get normalized right trigger value (0.0–1.0).  
---@return number  
function EasyInputHandler.GetRightTriggerNormalized() end

---Set gamepad vibration (dual motors).  
---@param left integer Left motor strength (0–65535)  
---@param right integer Right motor strength (0–65535)  
---@return boolean Success  
function EasyInputHandler.SetGamepadVibration(left, right) end

---Get every virtual key currently held down.  
---@return integer[] Virtual key codes  
function EasyInputHandler.GetPressedKeys() end

---Get a display name for a virtual key, e.g. 0x2D -> "Insert", 0x24 -> "Home".  
---@param vkCode integer Virtual key code  
---@return string  
function EasyInputHandler.GetKeyName(vkCode) end

---Start collecting typed text (WM_CHAR). Backspace, Enter and Escape arrive as "\b", "\n" and "\x1b".  
function EasyInputHandler.BeginTextCapture() end

---Stop collecting typed text and clear the buffer.  
function EasyInputHandler.EndTextCapture() end

---Return the UTF-8 text typed since the last call and clear it.  
---@return string  
function EasyInputHandler.ConsumeTypedText() end

---Cursor X in the game window, scaled to the render resolution (same space as GetDisplayResolution).  
---@return number  
function EasyInputHandler.GetMouseX() end

---Cursor Y in the game window, scaled to the render resolution (same space as GetDisplayResolution).  
---@return number  
function EasyInputHandler.GetMouseY() end

---Check a mouse button: 0 left, 1 right, 2 middle (respects swapped buttons).  
---@param button integer  
---@return boolean  
function EasyInputHandler.IsMouseButtonDown(button) end

---Wheel notches since the last call, positive is up.  
---@return number  
function EasyInputHandler.ConsumeMouseWheel() end

---Write window, WndProc and raw input details to the RED4ext plugin log.  
function EasyInputHandler.LogInputDiagnostics() end

---Get the label of a gamepad button flag for the connected controller type, e.g. 0x1000 -> "A" or "Cross".  
---@param buttonCode integer Gamepad button flag  
---@return string  
function EasyInputHandler.GetGamepadButtonName(buttonCode) end
