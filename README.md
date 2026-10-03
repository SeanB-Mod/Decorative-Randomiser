# Decorative Randomiser for Two Point Museum

Decorative Randomiser adds native-style placement controls to the Items menu, making it easy to create more varied displays while decorating a museum.

## Features

- Random rotation for every new item placement.
- Random colour across the full spectrum for items that support tinting.
- Random brightness for items that support tinting.
- Rotation, colour, and brightness can be enabled independently.
- Snapped items use half their normal rotation interval for additional variety.
- Free-placement items can use any rotation angle.
- A shuffled twelve-section hue system prevents long runs of similar blue or green colours.
- Colour saturation varies from 30–100%.
- Brightness varies from 30–100%.
- Changing to another colourable item works immediately without opening its colour picker.
- Existing items are not changed when moved.
- Native-style buttons, selected states, icons, spacing, and ON/OFF tooltips.
- Randomisation controls reset to off when the Items menu is closed.
- Drag Demolition and Demolish Museum controls are available from the Items menu.
- Placement-panel visual cleanup removes stray translucent toolbar backgrounds.
- Runtime work is cached and throttled to avoid frame-rate drops.

## Installation
Install the Two Point Museum Mod Loader separately.

1. Close Two Point Museum.
2. Copy `DecorativeRandomiser.dll` into the game's `Mods` folder.
3. Add it as the next numbered entry under `[Mods]` in `Mods\loader.ini`, for example:

   `6=DecorativeRandomiser.dll`

4. Start the game and open the Items menu.

If upgrading from an earlier build, delete `TPM-RandomPlacement.dll`, copy in `DecorativeRandomiser.dll`, and update its filename in `loader.ini`. Do not keep both DLLs.

## Removal

Close the game, remove the DLL's entry from `loader.ini`, and delete `DecorativeRandomiser.dll`. The optional `Mods\Saves\RandomPlacement.ini` settings file can also be deleted.

## Compatibility

Decorative Randomiser 1.0 was built and tested with Two Point Museum `12.0.243206+2026-09-29.2232` and TPM Multi-Mod Loader. It does not patch game code; Unity and IL2CPP calls are marshalled to the game's window thread.

## Building from source

The source pack requires 64-bit LLVM-MinGW on Windows. Add `clang++.exe` to PATH and run `src\build.ps1`, or supply its full path with `src\build.ps1 -Compiler C:\path\to\clang++.exe`. The script builds and runs the logic tests before producing `build\DecorativeRandomiser.dll`.
