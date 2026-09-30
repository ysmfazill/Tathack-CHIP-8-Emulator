# CHIP-8 Emulator

A CHIP-8 emulator built in C++ utilizing SDL2 for rendering and ImGui for an advanced debugging and control interface.

## Features
- Full execution of all 35 CHIP-8 opcodes
- 60 FPS capped rendering with real-time CPU speed control (1-30 instructions per frame)
- Interactive ImGui-powered ROM Browser
- Advanced Audio Customization Engine (Square, Sine, Triangle, Sawtooth waveforms)
- Live CPU Inspector and real-time Disassembler/Debugger
- Four selectable color palettes (Classic, Retro, Matrix, High Contrast)
- Pixel Grid and Blinky Game Assist overlays
- Per-ROM binary serialization save/load states with "Ghost" visual preview

## Bugs Fixed
1. **FX0A Key Wait**: PC is now strictly blocked until valid input is received.
2. **Timer Decoupling**: Delay and Sound timers now accurately decrement at 60Hz.
3. **Frame Timing**: The main loop uses adaptive frame delay instead of instruction-level throttling.
4. **DXYN Wrapping**: Sprites correctly wrap around the 64x32 screen boundaries.
5. **8XY5 / 8XY7 Subtraction**: The `VF` borrow flag safely handles equal values using the `>=` operator.
6. **FX55 / FX65 Bounds**: Register dumps include the final `Vx` register.

## Controls
- `ESC` - Quit emulator
- `UP Arrow` - Increase emulation speed
- `DOWN Arrow` - Decrease emulation speed
- `F1` - Classic Palette (White/Black)
- `F2` - Retro Palette (Amber)
- `F3` - Matrix Palette (Neon)
- `F4` - High Contrast Palette (Pure B&W)

```text
Chip-8 Keypad:          QWERTY Keyboard:
┌─┬─┬─┬─┐               ┌─┬─┬─┬─┐
│1│2│3│C│               │1│2│3│4│
├─┼─┼─┼─┤               ├─┼─┼─┼─┤
│4│5│6│D│               │Q│W│E│R│
├─┼─┼─┼─┤      =        ├─┼─┼─┼─┤
│7│8│9│E│               │A│S│D│F│
├─┼─┼─┼─┤               ├─┼─┼─┼─┤
│A│0│B│F│               │Z│X│C│V│
└─┴─┴─┴─┘               └─┴─┴─┴─┘
```

## Save States
- `F5` - Save state
- `F9` - Load state
States are saved to dedicated per-ROM binary files (e.g. `savestates/TETRIS.sav`).

## Audio
The emulator leverages `SDL_Audio` running safely on a separate thread, driven by the native CHIP-8 60Hz `sound_timer`. Through the UI, users can manipulate the frequency (100-2000Hz), volume (0-100%), and active waveform (Square, Sine, Triangle, Sawtooth) in real-time.

## Debugger
An integrated Disassembler accurately translates opcodes into human-readable assembly. 
- `F6` - Toggle Debug Mode (halts CPU)
- `F7` - Step forward one instruction
- `D` - Dump all registers to standard output
- `SPACE` - Resume execution

## ROM Browser
The application launches into an interactive graphical ImGui ROM browser out of the box, dynamically listing all `.ch8` files inside the `roms/` folder for one-click loading.

## Project Structure
```
Tathack-CHIP-8-Emulator/
├── src/
├── roms/
├── imgui/
├── Makefile
├── README.md
├── IMPLEMENTATION.md
├── AUDIT_RESULTS.md
├── LICENSE
└── .gitignore
```

## Build
```bash
mingw32-make clean
mingw32-make
```

## Run
**Launch with Graphical ROM Browser:**
```bash
./chip8
```
*(On Windows PowerShell, use `.\chip8.exe`)*

**Launch directly via Command Line:**
```bash
./chip8 roms/Pong.ch8
```
*(On Windows PowerShell, use `.\chip8.exe roms/Pong.ch8`)*

## Testing
- **Pong** — Verified locally on MSYS2
- **Tetris** — Verified locally on MSYS2
- **Blinky** — Verified locally on MSYS2

## Repository
https://github.com/ysmfazill/Tathack-CHIP-8-Emulator
