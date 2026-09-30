# CHIP-8 Emulator Implementation Details

## 1. Overview
This document provides a technical breakdown of the CHIP-8 emulator architecture, the bugs fixed, and the features implemented during development.

## 2. CHIP-8 Architecture
The CHIP-8 is a virtual machine utilizing:
- **Memory**: 4 KB (4096 bytes). `0x000-0x1FF` reserved for fonts. `0x200` onwards for ROM space.
- **Registers**: 16 general-purpose 8-bit registers (`V0`-`VF`), a 16-bit index register (`I`), a 16-bit Program Counter (`PC`), and an 8-bit Stack Pointer (`SP`).
- **Display**: 64x32 monochrome pixels.
- **Timers**: Two 60 Hz timers (Delay and Sound).
- **Stack**: 16 levels for subroutine calls.

## 3. Bug Fixes
1. **FX0A Key Wait**: Fixed non-blocking key wait to strictly block the PC until a key is pressed, returning early from `emulate_cycle()`.
2. **Timer Decoupling**: Decoupled `delay_timer` and `sound_timer` from the CPU instruction cycle loop. They are now decremented in the main loop at exactly 60 Hz.
3. **Frame Timing**: Moved `SDL_Delay` to the main loop to strictly enforce 60 FPS (16ms per frame) instead of throttling per instruction.
4. **DXYN Sprite Wrapping**: Applied bitwise AND (`& 63` and `& 31`) to sprite X and Y coordinates to correctly wrap around the screen edges.
5. **8XY5 / 8XY7 Subtraction**: Replaced `>` with `>=` to ensure the `VF` borrow flag is correctly set on equal values.
6. **FX55 / FX65 Bounds**: Updated loop condition to `<= X` to include the final `Vx` register during memory transfers.

## 4. CPU Implementation
The CPU is encapsulated in the `Chip8` class (`chip8.h`/`chip8.cpp`). It fetches a 16-bit opcode from memory using `PC` and decodes it using a `switch` statement bitmasked against the first nibble (`opcode & 0xF000`).

## 5. Timer Implementation
Timers are decremented dynamically via an accumulator in the main loop (`main.cpp`). 
```cpp
timer_accumulator += (elapsed_time / 1000.0f);
if (timer_accumulator >= (1.0f / 60.0f)) {
    chip8.decrease_delay_timer();
    chip8.decrease_sound_timer();
    timer_accumulator -= (1.0f / 60.0f);
}
```

## 6. Rendering Implementation
Rendering relies on `SDL2`. The 64x32 monochrome display is scaled up by 10x. The `draw_graphics` function maps `1` pixels to the selected ImGui color theme and `0` pixels to the background.

## 7. Input Handling
SDL keyboard events are intercepted in `handle_input()`. Keys are mapped from QWERTY to the original CHIP-8 16-key hex pad. The state is written to the `chip8.key[16]` array.

## 8. Save-State Implementation
Save states capture the exact struct state to binary per-ROM files (e.g. `savestates/TETRIS.sav`).
The `SaveState` structure matches `chip8.h`:
```cpp
struct SaveState {
    uint8_t memory[4096];
    uint8_t v[16];
    uint16_t index;
    uint16_t pc;
    uint16_t stack[16];
    uint8_t sp;
    uint8_t delay_timer;
    uint8_t sound_timer;
    uint32_t display[64 * 32];
    char rom_name[64];
    uint32_t version;
};
```

## 9. Audio / AudioState
The Audio engine leverages `SDL_Audio`. The `AudioState` struct is thread-safe using `std::atomic` values for `beeping`, `test_beeping`, `waveform`, `frequency`, and `volume`. The audio callback reads these atomics via `.load()` safely before synthesis, supporting Square, Sine, Triangle, and Sawtooth generation clamped safely within limits (100-2000Hz).

## 10. ImGui Control Overlay
Provides a user interface to control CPU speed (`cycles_per_frame`), toggle the Pixel Grid, save/load states, toggle the Save State Ghost overlay, change color palettes, and configure audio settings.

## 11. CPU Inspector
A secondary ImGui window rendering the real-time execution state of the emulator: Opcode, PC, SP, Timers, and V0-VF register values.

## 12. Disassembler / Debugger
A debug loop in `main.cpp` triggered by `F6`. It halts PC execution until `F7` is pressed (Step). `disassemble_opcode(uint16_t opcode)` translates the current instruction into human-readable assembly. `D` dumps current registers to stdout.

## 13. ROM Browser
Automatically scans the `roms/` directory and populates an ImGui selection menu if the emulator is run without command-line arguments. Clicking a ROM loads it dynamically and switches state to `PLAYING`.

## 14. Blinky Game Assist
Includes a score tracker displayed in ImGui when playing `BLINKY`, isolated entirely from the `Chip8` core CPU logic.

## 15. Build Environment
Compiled via `mingw32-make` utilizing MSYS2 UCRT64 toolchains (`g++`). Dependencies include `SDL2` and `ImGui`.

## 16. Testing
Testing verifies bug fixes and features on Pong, Tetris, and Blinky, specifically checking for F5/F9 state serialization and visual sprite boundaries.

## 17. Known Limitations
- The Disassembler only covers standard opcodes and lacks advanced stepping features (no breakpoints or memory viewer).
- Audio is limited to standard mono synthesis without advanced hardware filters.
