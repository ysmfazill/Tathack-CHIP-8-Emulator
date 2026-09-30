# CHIP-8 Emulator - Full Project Documentation

This document serves as the comprehensive final changelog and technical documentation for the TatHack '26 CHIP-8 Emulator. It details every bug fixed, every core feature added, and every advanced UI/UX addition implemented during the development cycle, alongside the relevant code changes.

---

## 🐛 PART 1: The 6 Core Bug Fixes

### 1. FX0A - Key Wait Blocking
**Issue:** The Program Counter (PC) advanced even when no key was pressed, breaking game loops.
**Fix:** Modified the instruction to strictly block the PC until a valid keypress is detected.
```cpp
// CHANGED IN chip8.cpp
case 0x000A: {
    bool key_pressed = false;
    for (int i = 0; i < 16; ++i) {
        if (key[i] != 0) {
            v[(opcode & 0x0F00) >> 8] = i;
            key_pressed = true;
            break;
        }
    }
    if (!key_pressed) {
        pc -= 2; // Blocks PC from advancing
    }
    break;
}
```

### 2. Timer Decoupling (60 Hz)
**Issue:** Timers decremented at CPU speed (e.g., 600 Hz), breaking game logic and sound durations.
**Fix:** Decoupled `delay_timer` and `sound_timer` from `emulate_cycle()`. They now run strictly in the main 60 FPS loop.
```cpp
// CHANGED IN main.cpp
while (timer_accumulator >= (1000.0f / 60.0f)) {
    chip8.decrease_delay_timer();
    chip8.decrease_sound_timer();
    timer_accumulator -= (1000.0f / 60.0f);
}
```

### 3. Frame Timing (60 FPS Lock)
**Issue:** The emulator ran unbounded, maxing out CPU and causing games to run impossibly fast.
**Fix:** Implemented an `SDL_Delay` calculation based on `SDL_GetTicks()` to stabilize at 60 FPS.
```cpp
// CHANGED IN main.cpp
const uint32_t TARGET_FRAME_TIME = 16; // ~60 FPS
uint32_t elapsed_time = SDL_GetTicks() - frame_start;
if (elapsed_time < TARGET_FRAME_TIME) {
    SDL_Delay(TARGET_FRAME_TIME - elapsed_time);
}
```

### 4. DXYN - Sprite Wrapping
**Issue:** Sprites drawn off-screen crashed the emulator or corrupted memory instead of cleanly wrapping around the 64x32 grid.
**Fix:** Applied bitwise limits `& 63` and `& 31` to X and Y coordinates respectively.
```cpp
// CHANGED IN chip8.cpp
case 0x0000: // DXYN
    uint8_t x = v[(opcode & 0x0F00) >> 8] & 63; // Wrap X
    uint8_t y = v[(opcode & 0x00F0) >> 4] & 31; // Wrap Y
    // ... sprite drawing logic
```

### 5. 8XY5 & 8XY7 - Subtraction Borrow Flag
**Issue:** The VF (borrow flag) logic used strict greater-than `>` instead of greater-than-or-equal `>=`, causing incorrect math.
**Fix:** Updated to `>=` and ensured VF is set *after* the subtraction to avoid overwriting VF when `X == 0xF`.
```cpp
// CHANGED IN chip8.cpp
case 0x0005: { // 8XY5 (Vx = Vx - Vy)
    uint8_t vx = v[(opcode & 0x0F00) >> 8];
    uint8_t vy = v[(opcode & 0x00F0) >> 4];
    v[(opcode & 0x0F00) >> 8] = vx - vy;
    v[0xF] = (vx >= vy) ? 1 : 0; // Corrected to >=
    break;
}
```

### 6. FX55 & FX65 - Memory Bounds
**Issue:** Register dumping/loading stopped one register early, failing to save/load VX.
**Fix:** Updated the loop conditional from `< X` to `<= X`.
```cpp
// CHANGED IN chip8.cpp
case 0x0065: {
    uint8_t X = (opcode & 0x0F00) >> 8;
    for (int i = 0; i <= X; ++i) { // Changed < to <=
        v[i] = memory[index + i];
    }
    break;
}
```

---

## 🚀 PART 2: Core Mandatory Features

### 1. Speed Control
Users can scale the CPU emulation speed linearly via the `UP` and `DOWN` arrow keys (1 to 30 cycles per frame).
```cpp
// ADDED IN main.cpp
else if (event.key.keysym.sym == SDLK_UP) {
    cycles_per_frame++;
    if (cycles_per_frame > 30) cycles_per_frame = 30;
}
```

### 2. Per-ROM Save/Load States
Implemented deep binary serialization of the CHIP-8 CPU into `savestates/<ROM>.sav`. Includes a safety header to strictly prevent loading a Tetris save while playing Pong. The loading system returns rich error codes (`1` = Success, `0` = Missing/Corrupt, `-1` = Wrong ROM) to provide precise persistent ImGui feedback. Global hotkeys (`F5` and `F9`) were relocated to trigger *before* ImGui's keyboard capture, guaranteeing savestates work reliably even when UI panels are focused.
```cpp
// ADDED IN chip8.cpp
int Chip8::load_state(const std::string& filename, const std::string& current_rom_name) {
    // ... read file ...
    if (state.version != 1 || std::string(state.rom_name) != current_rom_name) {
        return -1; // Safely rejects mismatched states
    }
    std::memcpy(memory, state.memory, sizeof(memory));
    // ... restore all registers ...
    return 1;
}
```

### 3. Color Palettes
Four live-switchable color themes natively modifying `SDL_SetRenderDrawColor` via `F1-F4` keys or ImGui radio buttons.
- Classic (White/Black)
- Retro (Amber/Dark Amber)
- Matrix (Neon Green/Dark Green)
- High Contrast (Pure B&W)

---

## 🛠️ PART 3: Advanced UX / Extras (Presentation Layer)

### 1. CPU Inspector UI
A real-time ImGui dashboard exposing the internal CPU state (V0-VF, PC, I, SP, Timers, Opcode) by changing private variables to public in `chip8.h`.
```cpp
// ADDED IN main.cpp
ImGui::Text("Opcode: 0x%04X", chip8.opcode);
ImGui::Text("PC: 0x%03X   I: 0x%03X", chip8.pc, chip8.index);
for (int i = 0; i < 8; i++) {
    ImGui::Text("V%X: %02X     V%X: %02X", i, chip8.v[i], i+8, chip8.v[i+8]);
}
```

### 2. Save State Ghost Overlay
Renders the currently saved `.sav` state as a highly-transparent alpha-blended layer beneath the active gameplay, allowing users to visually see their saved snapshot.
```cpp
// ADDED IN main.cpp
if (enable_ghost && ghost_state) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 64); // 25% opacity
    // ... draw ghost_state->display
}
```

### 3. Dynamic Audio Settings Generator & Thread-Safety
Ripped out the static CHIP-8 beep and implemented a realtime software synthesizer inside the `SDL_Audio` callback. Allows live toggling of Square, Sine, Triangle, and Sawtooth waves with configurable Hz and Volume. The `AudioState` struct was upgraded to utilize `<atomic>` fields, ensuring thread-safe communication between the UI configuration variables and the SDL audio rendering thread. Furthermore, the CHIP-8 sound timer is sampled *before* decrementing, resolving a regression where 1-frame sound effects (like in Tetris) were silenced before the audio thread could fire.
```cpp
// ADDED IN main.cpp
struct AudioState {
    std::atomic<bool> beeping{false};
    std::atomic<int> waveform{0};
    std::atomic<int> frequency{440};
    std::atomic<int> volume{50};
    // ...
};
```

### 4. Blinky Game Assist & Game Over Flow
An optional layer that securely parses `chip8.v` bounding boxes to calculate precise Pac-Man/Ghost collisions in `main.cpp` (leaving the emulator core entirely generic). When a collision triggers, it flashes a "CAUGHT!" HUD, increments a custom UI score, and automatically resets the Pac-Man sprite to its starting position. Upon expiration of the CAUGHT timer, it dynamically enters a "GAME OVER" state that prevents further score increments. The player can press `R` or `ENTER` to securely hot-reload the ROM binary and restart the gameplay loop from scratch.
```cpp
// ADDED IN main.cpp
if (rom_name == "BLINKY" && blinky_assist && !blinky_game_over) {
    if (std::abs(px - g1x) < 4 && std::abs(py - g1y) < 4) collision = true;
    if (collision && !blinky_collision_latch) {
        blinky_score += 100;
        chip8.v[8] = 0x1A; // Reset Pac-Man X
        chip8.v[9] = 0x0C; // Reset Pac-Man Y
        // Timer eventually sets blinky_game_over = true
    }
}
```

---

## 🧹 PART 4: Repository Hygiene
- Re-wrote `.gitignore` to comprehensively hide `*.exe`, `*.o`, `imgui.ini`, and the new runtime `savestates/` directory.
- Scrubbed previously tracked compilation artifacts from the Git index.
- Standardized the presentation of all ImGui windows to never obscure the primary 64x32 CHIP-8 framebuffer display.
