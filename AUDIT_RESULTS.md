# Final Audit & Implementation Details

## Bug Fixes

### 1. FX0A - Key Wait Blocking
**Issue:** The emulator's Program Counter (PC) was advancing even when no key was pressed, breaking key-dependent game mechanics.
**Original Broken Code:**
```cpp
case 0x000A: {
    bool key_pressed = false;
    for (int i = 0; i < 16; i++) {
        if (key[i] != 0) {
            v[(opcode & 0x0F00) >> 8] = i;
            key_pressed = true;
        }
    }
    pc += 2; // BUG: Always incremented PC!
    break;
}
```
**Fixed Code:**
```cpp
case 0x000A: {
    bool key_pressed = false;
    for (int i = 0; i < 16; i++) {
        if (key[i] != 0) {
            v[(opcode & 0x0F00) >> 8] = i;
            key_pressed = true;
            break;
        }
    }
    if (!key_pressed) return; // FIXED: Block PC until key is pressed
    pc += 2;
    break;
}
```

### 2. Timer Decoupling
**Issue:** Timers were decrementing at 600 Hz (every CPU cycle) instead of 60 Hz (once per frame), causing games to run too fast.
**Original Broken Code (in `emulate_cycle`):**
```cpp
// BUG: Decremented per cycle
if (delay_timer > 0) --delay_timer;
if (sound_timer > 0) --sound_timer;
```
**Fixed Code:**
Timers were removed from `emulate_cycle()` and moved to `decrease_delay_timer()` and `decrease_sound_timer()` methods, which are called exactly 60 times per second in `main.cpp` using a timer accumulator.

### 3. Frame Timing (60 FPS)
**Issue:** Hardcoded `SDL_Delay(16)` inside the CPU cycle loop caused the emulator to run extremely slowly.
**Original Broken Code (in `main` loop):**
```cpp
for (int i = 0; i < cycles_per_frame; i++) {
    chip8.emulate_cycle();
    SDL_Delay(16); // BUG: Throttled every single cycle!
}
```
**Fixed Code:**
```cpp
for (int i = 0; i < cycles_per_frame; i++) {
    chip8.emulate_cycle();
}
// FIXED: Delay once per frame
uint32_t elapsed_time = SDL_GetTicks() - frame_start;
if (elapsed_time < TARGET_FRAME_TIME) {
    SDL_Delay(TARGET_FRAME_TIME - elapsed_time);
}
```

### 4. DXYN Sprite Wrapping
**Issue:** Sprites drawn at the edges of the screen disappeared instead of wrapping around.
**Original Broken Code:**
```cpp
int pixel_x = x + xline;
int pixel_y = y + yline;
// BUG: No wrapping bounds check!
```
**Fixed Code:**
```cpp
// FIXED: Wrap using bitwise AND
int pixel_x = (x + xline) & 63;
int pixel_y = (y + yline) & 31;
```

### 5. Arithmetic Flags (8XY5 / 8XY7)
**Issue:** The Borrow flag calculation incorrectly used `>` instead of `>=`, failing on edge cases where `Vx == Vy`.
**Original Broken Code:**
```cpp
case 0x8005:
    // BUG: Used >
    v[0xF] = (v[(opcode & 0x0F00) >> 8] > v[(opcode & 0x00F0) >> 4]) ? 1 : 0;
    v[(opcode & 0x0F00) >> 8] -= v[(opcode & 0x00F0) >> 4];
    pc += 2;
    break;
```
**Fixed Code:**
```cpp
case 0x8005:
    // FIXED: Used >=
    v[0xF] = (v[(opcode & 0x0F00) >> 8] >= v[(opcode & 0x00F0) >> 4]) ? 1 : 0;
    v[(opcode & 0x0F00) >> 8] -= v[(opcode & 0x00F0) >> 4];
    pc += 2;
    break;
```

### 6. FX55 / FX65 Bounds
**Issue:** Loop conditional updated to `< X`, excluding the final register transfer.
**Original Broken Code:**
```cpp
// BUG: Loop misses final register
for (int i = 0; i < ((opcode & 0x0F00) >> 8); i++) {
```
**Fixed Code:**
```cpp
// FIXED: Loop includes final register
for (int i = 0; i <= ((opcode & 0x0F00) >> 8); ++i) {
```

---

## Features Implemented

### 1. Speed Control
Added `cycles_per_frame` dynamically controlled by `UP/DOWN` arrows.
```cpp
else if (event.key.keysym.sym == SDLK_UP) {
    if (cycles_per_frame < 30) cycles_per_frame++;
}
else if (event.key.keysym.sym == SDLK_DOWN) {
    if (cycles_per_frame > 1) cycles_per_frame--;
}
```

### 2. Savestate & Loadstate
Binary serialization of `SaveState` via `F5` (Save) and `F9` (Load).
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

### 3. Color Palettes
4 selectable rendering themes (Classic, Retro, Matrix, High Contrast) via ImGui dashboard. Handled by passing `color_theme` to `draw_graphics()` which sets `SDL_SetRenderDrawColor` accordingly.

### 4. Disassembler & Debugger (Bonus)
Added `disassemble_opcode(uint16_t opcode)` which outputs instructions in human-readable assembly. Included a debug loop that halts the PC until `F7` (Step) is pressed, outputting register dumps when `D` is pressed.
```cpp
if (debug_mode && step_requested) {
    std::cout << "[PC: 0x" << std::hex << chip8.pc << "] " 
              << disassemble_opcode(current_opcode) << std::endl;
    chip8.emulate_cycle();
    step_requested = false;
}
```

### 5. ROM File Browser UI (Bonus)
Scans the `roms/` directory and renders a complete ImGui window at boot to select ROMs interactively without the command line.
```cpp
if (ImGui::Begin("ROM Browser")) {
    for(size_t i = 0; i < available_roms.size(); i++) {
        if(ImGui::Selectable(available_roms[i].c_str(), selected_index == i)) {
            selected_rom = "roms/" + available_roms[selected_index];
            chip8.load_rom(selected_rom);
            state = PLAYING;
        }
    }
    ImGui::End();
}
```

## Runtime Tests
Pong: Verified locally on MSYS2 by user
Tetris: Verified locally on MSYS2 by user
Blinky: Verified locally on MSYS2 by user

## Build
Command: `mingw32-make clean && mingw32-make`
Result: Passes locally.

## Keyboard Mapping (Reference)

### Standard Controls
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
- `ESC` - Quit emulator

### Extended Feature Controls
- `UP Arrow` - Increase emulation speed (cycles per frame)
- `DOWN Arrow` - Decrease emulation speed (cycles per frame)
- `F5` - Save state to savestates/<ROM>.sav
- `F9` - Load state from savestates/<ROM>.sav
- `F1` - Classic Palette (White/Black)
- `F2` - Retro Palette (Amber)
- `F3` - Matrix Palette (Neon)
- `F4` - High Contrast Palette (Pure B&W)

### Debugger Controls (Bonus)
- `F6` - Toggle Debug Mode (pause/resume)
- `F7` - Step forward one instruction
- `D` - Dump all registers to standard output
- `SPACE` - Resume normal play

### ROM File Browser UI (Bonus)
- **Automatic** - Appears on startup if no ROM is passed via command line.
- **Mouse Left-Click** - Select a ROM from the list and click "Load ROM".
