local EasyInput = nil
local function GetEasyInput()
    if not EasyInput then
        EasyInput = EasyInputHandler.new()
    end
    return EasyInput
end

-- Keyboard VK codes
local VK_SPACE = 0x20
local VK_A = 0x41
local VK_D = 0x44

-- Gamepad button flags
local GP_A = 0x1000
local GP_B = 0x2000
local GP_X = 0x4000
local GP_Y = 0x8000
local GP_LB = 0x0100
local GP_RB = 0x0200
local GP_BACK = 0x0020
local GP_START = 0x0010
local GP_LS = 0x0040
local GP_RS = 0x0080
local GP_DPAD_UP = 0x0001
local GP_DPAD_DOWN = 0x0002
local GP_DPAD_LEFT = 0x0004
local GP_DPAD_RIGHT = 0x0008

-- Stick direction set
local directions = {
    "up", "down", "left", "right",
    "upleft", "upright", "downleft", "downright"
}

-- State
local rumbleLeft, rumbleRight = 0, 0
local rumbleActive = false
local cursorVisible = false

registerForEvent("onDraw", function()
    local input = GetEasyInput()
    ImGui.Begin("EasyInput Full Test")

    -- Keyboard
    ImGui.Text("Keyboard Active: " .. (input:IsKeyboardActive() and "Yes" or "No"))
    ImGui.Text("Last Key: " .. input:GetPressedKey())
    ImGui.Text(string.format("Space:%s  A:%s  D:%s",
        input:IsKeyPressed(VK_SPACE), input:IsKeyPressed(VK_A), input:IsKeyPressed(VK_D)
    ))

    -- Spacebar + button toggle just to mess around with the cursor
    if input:IsKeyPressed(VK_SPACE) then
        cursorVisible = not cursorVisible
    end
    if ImGui.Button(cursorVisible and "Hide Cursor" or "Show Cursor") then
        cursorVisible = not cursorVisible
    end
    input:ShowCursor(cursorVisible)
    ImGui.Text("Cursor: " .. (input:IsCursorVisible() and "Visible" or "Hidden"))

    -- Gamepad
    ImGui.Separator()
    ImGui.Text("Gamepad Active: " .. (input:IsGamepadActive() and "Yes" or "No"))
    ImGui.Text("Last Button: " .. input:GetPressedGamepadButton())

    ImGui.Text(string.format("A:%s  B:%s  X:%s  Y:%s",
        input:IsGamepadButtonPressed(GP_A), input:IsGamepadButtonPressed(GP_B),
        input:IsGamepadButtonPressed(GP_X), input:IsGamepadButtonPressed(GP_Y)
    ))
    ImGui.Text(string.format("LB:%s  RB:%s  LS:%s  RS:%s",
        input:IsGamepadButtonPressed(GP_LB), input:IsGamepadButtonPressed(GP_RB),
        input:IsGamepadButtonPressed(GP_LS), input:IsGamepadButtonPressed(GP_RS)
    ))
    ImGui.Text(string.format("Back:%s  Start:%s", input:IsGamepadButtonPressed(GP_BACK), input:IsGamepadButtonPressed(GP_START)))
    ImGui.Text(string.format("DPad U:%s  D:%s  L:%s  R:%s",
        input:IsGamepadButtonPressed(GP_DPAD_UP), input:IsGamepadButtonPressed(GP_DPAD_DOWN),
        input:IsGamepadButtonPressed(GP_DPAD_LEFT), input:IsGamepadButtonPressed(GP_DPAD_RIGHT)
    ))

    -- Stick directions
    local stickLine = {}
    for _, dir in ipairs(directions) do
        table.insert(stickLine, dir .. ":" .. tostring(input:IsLeftStickDirection(dir)))
    end
    ImGui.Text("Left Stick: " .. table.concat(stickLine, " | "))

    stickLine = {}
    for _, dir in ipairs(directions) do
        table.insert(stickLine, dir .. ":" .. tostring(input:IsRightStickDirection(dir)))
    end
    ImGui.Text("Right Stick: " .. table.concat(stickLine, " | "))

    -- Axes
    ImGui.Separator()
    ImGui.Text(string.format("LX:%s  LY:%s  RX:%s  RY:%s",
        input:GetGamepadAxis(0), input:GetGamepadAxis(1),
        input:GetGamepadAxis(2), input:GetGamepadAxis(3)
    ))

    -- Triggers
    ImGui.Text(string.format("LT:%d (%.2f)  RT:%d (%.2f)",
        input:GetLeftTrigger(), input:GetLeftTriggerNormalized(),
        input:GetRightTrigger(), input:GetRightTriggerNormalized()
    ))

    -- Vibration
    rumbleLeft, _ = ImGui.SliderInt("Left Motor", rumbleLeft, 0, 65535)
    rumbleRight, _ = ImGui.SliderInt("Right Motor", rumbleRight, 0, 65535)
    if ImGui.Button(rumbleActive and "Stop Rumble" or "Start Rumble") then
        rumbleActive = not rumbleActive
    end
    input:SetGamepadVibration(rumbleActive and rumbleLeft or 0, rumbleActive and rumbleRight or 0)

    ImGui.End()
end)
