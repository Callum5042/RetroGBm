# RetroGBm
Gameboy Colour emulator written in C++ for Windows and Android.

### Features

- Save States
- MBC1/MBC3/MBC5

## Screenshots

![Pokemon Gold](/Images/pokemon_gold.png)
![Zelda Link's Awakening DX](/Images/zelda_links_awakening_dx.png)
![Dr Mario](/Images/dr_mario.png)
![Pokemon Yellow](/Images/pokemon_yellow.png)
![Donkey Kong Land](/Images/donkey_kong_land.png)
![Super Mario Land](/Images/super_mario_land.png)
![Mario Tennis](/Images/mario_tennis.png)

## Build

**Windows**: Install Visual Studios 2022 and build from the solution file `RetroGBm.sln`.

**Android**: Install Android Studios and open the `android` folder, then build with Android Studios.

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
| Zelda - Link's Awakening DX | ★★★★★ |  |
| Pokemon Yellow    | ★★★★★ |  |
| Super Mario Land  | ★★★★☆ | Pause screen is incorrectly displayed |
| Pokemon Gold		| ★★★★☆ | RTC doesn't work |
| Pokemon Crystal	| ★★★★☆ | RTC doesn't work |

## Test roms

### Blargg

- [ ] cgb_sound
- [x] cpu_instrs
- [ ] dmg_sound
- [x] instr_timing.gb
- [ ] interrupt_time.gb
- [x] mem_timing.gb
- [x] mem_timing-2.gb
- [x] halt_bug

### Gekkio Mooneye

**acceptance**

- [x] add_sp_e_timing
- [x] call_cc_timing
- [x] call_cc_timing2
- [x] call_timing
- [x] call_timing2
- [x] div_timing
- [x] ei_sequence
- [x] ei_timing
- [x] halt_ime0_ei
- [x] halt_ime0_nointr_timing
- [x] halt_ime1_timing
- [x] if_ie_registers
- [x] intr_timing
- [x] jp_cc_timing
- [x] jp_timing
- [x] ld_hl_sp_e_timing
- [x] oam_dma_restart
- [ ] oam_dma_start
- [x] oam_dma_timing
- [x] pop_timing
- [x] push_timing
- [x] rapid_di_ei
- [x] ret_cc_timing
- [x] ret_timing
- [x] reti_intr_timing
- [x] reti_timing
- [x] rst_timing
- [x] bits/mem_oam
- [x] bits/reg_f
- [x] instr/daa
- [ ] interrupts/ie_push
- [x] oam_dma/basic
- [x] oam_dma/reg_read
- [ ] ppu/intr_2_0_timing
- [ ] ppu/intr_2_mode0_timing
- [ ] ppu/intr_2_mode0_timing_sprites
- [ ] ppu/intr_2_mode3_timing
- [ ] ppu/stat_irq_blocking
- [ ] ppu/stat_lyc_onoff
- [x] timer/div_write
- [ ] timer/rapid_toggle
- [x] timer/tim00
- [x] timer/tim00_div_trigger
- [x] timer/tim01
- [x] timer/tim01_div_trigger
- [x] timer/tim10
- [x] timer/tim10_div_trigger
- [x] timer/tim11
- [x] timer/tim11_div_trigger
- [x] timer/tima_reload
- [ ] timer/tima_write_reloading
- [ ] timer/tma_write_reloading

**emulator-only/mbc1**

- [x] bits_bank1
- [x] bits_bank2
- [x] bits_mode
- [x] bits_ramg
- [ ] multicart_rom_8Mb
- [x] ram_64kb
- [x] ram_256kb
- [x] rom_1Mb
- [x] rom_2Mb
- [x] rom_4Mb
- [x] rom_8Mb
- [x] rom_16Mb

**emulator-only/mbc5**

- [x] rom_512kb
- [x] rom_1Mb
- [x] rom_2Mb
- [x] rom_4Mb
- [x] rom_8Mb
- [x] rom_16Mb
- [x] rom_32Mb
- [x] rom_64Mb
- [x] rom_512kb

### Acid

- [x] dmg-acid2
- [x] cgb-acid2

![dmg-acid2](/Images/dmg_acid2.png)
![cgb-acid2](/Images/cgb_acid2.png)
