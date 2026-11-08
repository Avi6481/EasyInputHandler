# EasyInputHandler

A RED4ext plugin for Cyberpunk 2077 that gives CET mods keyboard, mouse, cursor and gamepad input. It's what the EasyTrainer menu runs on, but any CET mod can use it.

## What it does

- Reads any key, mouse buttons, the wheel and the cursor position
- Typing capture for text boxes
- Gamepads through SDL3, so Xbox, PlayStation, Switch and most other controllers work without Steam Input
- Shows the game's cursor
- Blocks keys, mouse or gamepad input from reaching the game while your menu uses them
- Rumble

## Install

Needs RED4ext and redscript. Built for game version 2.5.

```
red4ext/plugins/EasyTrainer/EasyInputHandler.dll
red4ext/plugins/EasyTrainer/SDL3.dll
r6/scripts/EasyTrainer/EasyInputHandler.reds
```

Without `SDL3.dll` the keyboard still works, only gamepads are off. The `.reds` file is required.

## Use it from CET

```lua
local input = EasyInputHandler.new()

if input:IsKeyPressed(0x0D) or input:IsGamepadButtonPressed(0x1000) then
    print("Enter or A pressed")
end

input:BlockKey(0x0D)
```

Blocks last 250 ms, so call them every frame you need them. `CET/EasyInputHandler.lua` has editor hints for every function and `CET/init.lua` is a small example mod. All functions, button codes and how blocking works are in [NOTES.md](NOTES.md).

## Build

1. Visual Studio 2022 or newer, x64.
2. Clone [RED4ext.SDK](https://github.com/wopss/RED4ext.SDK) next to this folder, so it sits at `../RED4ext.SDK`.
3. Set `GameDir` in `EasyTrainerKey/EasyTrainerKey.vcxproj` to your game folder, or pass `/p:GameDir=...`.
4. Build Release x64. The DLL, `SDL3.dll` and the `.reds` file get copied into the game folder.

## Contributing

Pull requests are welcome.

## Credits

Gamepad input uses [SDL3](https://www.libsdl.org) (zlib license, see `EasyTrainerKey/SDL3/LICENSE.txt`).

## License

MIT, see [LICENSE](LICENSE).
