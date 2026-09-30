# Chip-8 Emulator

A Chip-8 emulator built in C++ with SDL2 graphics and audio support.

> **Note to Participants:** 
> This codebase is intentionally incomplete and contains implementation defects across opcode handling, memory management, timing control, and rendering pipeline. Please consult the **Problem Statement** document for your exact submission guidelines and evaluation criteria.

## Features

- All 35 Chip-8 opcodes implemented
- 64x32 pixel display with SDL2 rendering
- Keyboard input support
- Sound effects (beep tone)
- 60 FPS rendering
- Speed Control (UP/DOWN arrows)
- Savestate/Loadstate (F5/F9)
- 4 Color Palettes (F1/F2/F3/F4 keys)

## Implementation Defects Identified and Fixed During Debugging

1. **FX0A Key Wait:** Fixed non-blocking key wait to strictly block the PC until a key is pressed.
2. **Timer Decoupling:** Decoupled delay/sound timers from the CPU cycle speed so they decrement strictly at 60 Hz.
3. **60 FPS Timing:** Fixed the main loop timing by adding adaptive SDL_Delay to maintain exactly 16ms per frame.
4. **DXYN Wrapping:** Fixed sprite rendering to properly wrap `X` and `Y` coordinates independently across screen boundaries.
5. **Arithmetic Flags (8XY5/8XY7):** Fixed subtract operations to correctly use `>=` instead of `>` when setting the VF borrow flag.
6. **FX55/FX65 Bounds:** Fixed memory load/store bounds to use `<=` instead of `<` to include the final `X` register loop iteration.

## Architecture

Chip-8 is a virtual machine from the 1970s designed to make programming video games easier on early microcomputers.

### System Specifications

- **Memory**: 4 KB (4096 bytes)
  - `0x000-0x1FF`: Reserved for interpreter and fonts
  - `0x200-0xFFF`: Program/ROM space
- **Registers**:
  - 16 8-bit general-purpose registers (V0-VF)
  - VF doubles as a flag register for arithmetic operations
  - 16-bit index register (I)
  - 16-bit program counter (PC)
  - 8-bit stack pointer (SP)
- **Display**: 64x32 pixels, monochrome
- **Timers**: 
  - Delay timer (counts down at 60 Hz)
  - Sound timer (beeps when > 0, counts down at 60 Hz)
- **Stack**: 16 levels for subroutine calls
- **Keypad**: 16-key hexadecimal input

## Dependencies

- SDL2 library

### Installation

**Arch Linux:**
```bash
sudo pacman -S sdl2
```

**Ubuntu/Debian:**
```bash
sudo apt-get install libsdl2-dev
```

**macOS:**
```bash
brew install sdl2
```

## Building

1. Clone the repo
```bash
git clone https://github.com/TatHack-Tathva/chip8-emulator.git
```

2. Make it
```bash
make
```

## Usage
```bash
./chip8 <path-to-rom-file>
```

**Example:**
```bash
./chip8 roms/PONG.ch8
```

## Keyboard Mapping

The original Chip-8 keypad is mapped to keyboard keys:
```
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

**Controls:**
- `ESC` - Quit emulator
- Keyboard keys as mapped above

**New Features:**
- `UP Arrow` - Increase emulation speed (cycles per frame)
- `DOWN Arrow` - Decrease emulation speed (cycles per frame)
- `F5` - Save state to savestates/<ROM>.sav
- `F9` - Load state from savestates/<ROM>.sav
- `F1` - Classic Palette (White/Black)
- `F2` - Retro Palette (Amber)
- `F3` - Matrix Palette (Neon)
- `F4` - High Contrast Palette (Pure B&W)

### Game-Specific Controls

**PONG:**
- Left paddle: `1` (up), `Q` (down)
- Right paddle: `4` (up), `R` (down)

**TETRIS:**
- `Q` - Rotate
- `W` - Drop
- `E` - Move right
- `A` - Move left

## Implementation Details

### Instruction Set

The emulator implements all 35 Chip-8 instructions, including:
- **Arithmetic**: ADD, SUB, AND, OR, XOR, shift operations
- **Graphics**: Draw sprites with XOR mode, collision detection
- **Flow control**: Jump, call/return subroutines, conditional skips
- **Memory**: Load/store registers, BCD conversion
- **Timers**: Delay and sound timer operations
- **Input**: Key press detection (blocking and non-blocking)

### Display

Graphics are rendered using SDL2:
- Each Chip-8 pixel is scaled 10× for visibility (640×320 window)
- XOR-based sprite drawing for collision detection
- 60 FPS rendering

### Audio

Simple square wave generation at 440 Hz (musical note A) plays when `sound_timer > 0`.

## Testing

The emulator has been rigorously tested against standard CHIP-8 test ROMs and actual games:

- **Opcode behavior:** Verified arithmetic flags (8XY5, 8XY7), correctly blocked key waits (FX0A), and safe memory bounds.
- **Sprite wrapping & collision:** Checked DXYN wrapping on screen boundaries using Pong. Verified collision flag (VF) flips correctly on XOR erase.
- **Timers:** Confirmed 60 Hz delay and sound timers operate smoothly regardless of CPU cycle speed.
- **Save/Load State:** Verified memory, registers, and timers are correctly serialized and deserialized via `savestates/<ROM>.sav` without corruption.
- **Speed control:** Tested real-time up/down scaling (1 to 30 CPU cycles per frame).
- **Games Tested:** Pong, Tetris, Blinky. All run at stable framerates with correct input handling.

## Resources

- [Chip-8 ROMs Archive](https://github.com/kripod/chip8-roms)
