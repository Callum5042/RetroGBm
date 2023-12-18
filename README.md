# RetroGBm
Gameboy Colour emulator written in C++

![Pokemon Yellow](/Images/pokemon_yellow.png)

## Missing Features
Currently Windows only and has no plans for sound support.

## Compatibility

| Rating		| Description |
| -----------	| ----------- |
| ★★★★★		| Perfect with no known issues |
| ★★★★☆		| Playable with minor graphical glitches |
| ★★★☆☆		| Major game breaking glitches |
| ★★☆☆☆		| Boots to title screen |
| ★☆☆☆☆		| Doesn't boot |

| Game				| Rating	| Notes				|
|-------------------|-----------|-------------------|
| Tetris			| ★★★★★ |  |
| Dr Mario		    | ★★★★★ |  |
| Pokemon Red		| ★★★★★ |  |
| Pokemon Yellow    | ★★★★☆ | Pokemon sprites aren't coloured correctly |
| Super Mario Land  | ★★★★☆ | Pause screen is incorrectly displayed |
| Pokemon Gold		| ★★★★☆ | Text boxes have some graphical issues while in a battle. RTC doesn't work |
| Zelda - Link's Awakening DX | ★★★★☆ | Background tiles occasionally turn white |
| Pokemon Crystal	| ★★☆☆☆ | Doesn't display the New Game window correctly |

## Test roms

- [x] Blargg cpu_instrs
- [x] Blargg instr_timing.gb
- [ ] Blargg interrupt_time.gb
- [x] Blargg mem_timing.gb
- [x] Blargg mem_timing-2.gb
- [x] dmg-acid2
- [x] cgb-acid2

![Blargg cpu_instrs](/Images/blargg_cpu_instrs.png)
![dmg-acid2](/Images/dmg_acid2.png)
![cgb-acid2](/Images/cgb_acid2.png)
