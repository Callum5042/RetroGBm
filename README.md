# RetroGBm

A fast and lightweight Game Boy Color emulator written in C++ for Windows and Android.

![Build](https://github.com/Callum5042/RetroGBm/actions/workflows/msbuild.yml/badge.svg)

### Features

- Support for MBC1, MBC3, and MBC5 mappers
- Battery-backed save support
- Multiple save states with rotating quick saves
- Adjustable emulation speed
- Real-Time Clock (RTC) emulation
- Dot-based PPU timing with variable pixel transfer and STAT interrupts
- Audio emulation
- CGB colours for DMG games, enabled by default, with a grayscale option on Windows and Android
- Play time tracking
- Link cable support
- Gameshark cheat codes

## Screenshots

| ![Pokemon Gold](/Images/pokemon_gold.png) | ![Zelda Link's Awakening DX](/Images/zelda_links_awakening_dx.png) | ![Dr Mario](/Images/dr_mario.png) |
|----------|----------|----------|
| ![Pokemon Yellow](/Images/pokemon_yellow.png) | ![Donkey Kong Land](/Images/donkey_kong_land.png)  | ![Super Mario Land](/Images/super_mario_land.png)  |
| ![Mario Tennis](/Images/mario_tennis.png) | ![Pokemon Pinball](/Images/pokemon_pinball.png)  | ![Zelda - Oracle of Ages](/Images/zelda_oracle_ages.png)  |

| ![Android List](/Images/android1.jpg) | ![Android Playing](/Images/android2.jpg) | ![Android Savestates](/Images/android3.jpg) |
|----------|----------|----------|

## Build

**Windows**: Install Visual Studios 2026 and build from the solution file `RetroGBm.sln`.

**Android**: Install Android Studios and open the `android` folder, then build with Android Studios.

## Compatibility

The **CGB Colours for DMG Games** option selects the Game Boy Color compatibility
palettes from the cartridge header (including Dr. Mario). Turn it off for DMG
grayscale. Native CGB games always use their own palettes. DMG games retain DMG
rendering rules in either setting. When the boot animation is enabled, DMG games
use the DMG boot ROM with the selected display palettes.

The new PPU uses save-state version 2. Older save states are rejected; battery
saves are unaffected.

[Compatibility](/Compatibility.md)

## Test roms

[Testing ROMS](/Testing.md)
