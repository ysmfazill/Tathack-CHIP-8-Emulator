# Final Audit

## Source Audit
Files inspected during final audit:
- src/main.cpp
- src/chip8.cpp
- src/chip8.h
- README.md
- IMPLEMENTATION.md
- AUDIT_RESULTS.md
- Makefile
- .gitignore

## Bug Fix Verification

1. **FX0A (Key Wait)**
   - Status: PASS
   - Evidence: `if (!key_pressed) return;` properly halts execution in `chip8.cpp` until input is detected.

2. **Timers**
   - Status: PASS
   - Evidence: Timers successfully decoupled from `emulate_cycle()` and decremented strictly based on `timer_accumulator` at 60Hz in `main.cpp`.

3. **Frame Timing**
   - Status: PASS
   - Evidence: Target frame time enforced dynamically using `SDL_GetTicks()` and `SDL_Delay` outside the CPU loop in `main.cpp`.

4. **DXYN (Sprite Wrapping)**
   - Status: PASS
   - Evidence: Coords properly wrapped via `int pixel_x = (x + xline) & 63;` and `int pixel_y = (y + yline) & 31;` in `chip8.cpp`.

5. **8XY5 / 8XY7 (Subtraction Flags)**
   - Status: PASS
   - Evidence: `v[0xF]` correctly uses `>=` operator for borrow logic.

6. **FX55 / FX65 (Bounds)**
   - Status: PASS
   - Evidence: Loop condition properly fixed to `i <= ((opcode & 0x0F00) >> 8)`.

## Feature Audit

- Speed Control: PASS
- Save State: PASS
- Load State: PASS
- Per-ROM state files: PASS
- Color palettes: PASS
- Pixel Grid: PASS
- ImGui Control Overlay: PASS
- Audio Settings: PASS
- CPU Inspector: PASS
- Disassembler: PASS
- Debugger: PASS
- ROM Browser: PASS
- Blinky Game Assist: PASS

## Runtime Testing

Pong — Verified locally on MSYS2
Tetris — Verified locally on MSYS2
Blinky — Verified locally on MSYS2

## Build

Command:
`mingw32-make clean && mingw32-make`
Result: Passes locally.

## Repository Hygiene
- `chip8.exe`, `*.o`, `imgui.ini`, `*.sav`, and `savestates/` are properly ignored in `.gitignore`.
- No generated binaries present in repo.

## Known Limitations
- The Disassembler only covers standard opcodes and lacks advanced stepping features (no breakpoints or memory viewer).
- Audio is limited to standard mono synthesis without advanced hardware filters.
