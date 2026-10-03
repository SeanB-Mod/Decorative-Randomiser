# Decorative Randomiser for Two Point Museum

Decorative Randomiser adds native-style placement controls to the Items menu, making it easy to create more varied displays while decorating a museum.

## Features

- Random rotation for every new item placement.
- Random colour across the full spectrum for items that support tinting.
- Colour Focused generates colours inside a circular area centred on a position chosen with the game's colour picker.
- The Random Colour button cycles through Off, Full Wheel, and Colour Focused modes; Colour Focused turns the button green.
- Random brightness across 40–100% for items that support tinting.
- Brightness Focused varies brightness by up to 15 percentage points above or below the picker value, clamped to 0–100%.
- Colour Focused and Brightness Focused update immediately when their picker values are changed on the active item.
- The Random Brightness button cycles through Off, 40–100%, and Brightness Focused modes; Brightness Focused turns the button green.
- Rotation, colour, and brightness can be enabled independently.
- Snapped items use half their normal rotation interval for additional variety.
- Free-placement items can use any rotation angle.
- Native-style buttons, selected states, icons, spacing, and ON/OFF tooltips.
- Randomisation controls reset to off when the Items menu is closed.
- Drag Demolition and Demolish Museum controls are available from the Items menu.

## Colour Focused

1. With Random Colour off, use the game's colour picker to choose the centre colour for the range.
2. Click Random Colour once for Full Wheel mode.
3. Click it again for Colour Focused. The button turns green.

Colour Focused selects points from a circle whose radius is 30% of the colour wheel. This varies both hue and saturation around the chosen colour, and clips the circle cleanly at the wheel's outer edge. Clicking the button once more turns Random Colour off.

## Brightness Focused

1. With Random Brightness off, use the game's colour picker to choose the centre brightness.
2. Click Random Brightness once for the full 40–100% range.
3. Click it again for Brightness Focused. The button turns green.

Brightness Focused selects a brightness within 15 percentage points either side of the chosen value, without exceeding 0% or 100%. Moving the brightness slider while it is active immediately changes the centre for subsequent placements. Clicking the button once more turns Random Brightness off.

## Installation

1. Close Two Point Museum.
2. Copy `TPM-DecorativeRandomiser.dll` into the game's `Mods` folder.
3. Add it as the next numbered entry under `[Mods]` in `Mods\loader.ini`, for example:

   `6=TPM-DecorativeRandomiser.dll`

4. Start the game and open the Items menu.

## Removal

Close the game, remove the DLL's entry from `loader.ini`, and delete `TPM-DecorativeRandomiser.dll`. The optional `Mods\Saves\RandomPlacement.ini` settings file can also be deleted.

## Compatibility

Decorative Randomiser 1.1 was built and tested with Two Point Museum `12.0.243206+2026-09-29.2232` and TPM Multi-Mod Loader. It does not patch game code; Unity and IL2CPP calls are marshalled to the game's window thread.

## Building from source

The source pack requires 64-bit LLVM-MinGW on Windows. Add `clang++.exe` to PATH and run `src\build.ps1`, or supply its full path with `src\build.ps1 -Compiler C:\path\to\clang++.exe`. The script builds and runs the logic tests before producing `build\TPM-DecorativeRandomiser.dll`.
