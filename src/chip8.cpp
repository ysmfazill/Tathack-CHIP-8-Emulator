#include "chip8.h"
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <random>
#include <cstdio>
#include <string>

std::string disassemble_opcode(uint16_t opcode) {
    char buffer[256];
    uint8_t x = (opcode & 0x0F00) >> 8;
    uint8_t y = (opcode & 0x00F0) >> 4;
    uint8_t n = opcode & 0x000F;
    uint8_t nn = opcode & 0x00FF;
    uint16_t nnn = opcode & 0x0FFF;
    
    switch(opcode & 0xF000) {
        case 0x0000:
            if(opcode == 0x00E0) return "00E0 - Clear display";
            if(opcode == 0x00EE) return "00EE - Return";
            return "0NNN - SYS";
        
        case 0x1000: snprintf(buffer, 256, "1%03X - Jump 0x%03X", nnn, nnn); return buffer;
        case 0x2000: snprintf(buffer, 256, "2%03X - Call 0x%03X", nnn, nnn); return buffer;
        case 0x3000: snprintf(buffer, 256, "3%X%02X - Skip if V%X==0x%02X", x, nn, x, nn); return buffer;
        case 0x4000: snprintf(buffer, 256, "4%X%02X - Skip if V%X!=0x%02X", x, nn, x, nn); return buffer;
        case 0x5000: snprintf(buffer, 256, "5%X%X0 - Skip if V%X==V%X", x, y, x, y); return buffer;
        case 0x6000: snprintf(buffer, 256, "6%X%02X - Set V%X=0x%02X", x, nn, x, nn); return buffer;
        case 0x7000: snprintf(buffer, 256, "7%X%02X - Add V%X+=0x%02X", x, nn, x, nn); return buffer;
        
        case 0x8000: {
            switch(n) {
                case 0x0: snprintf(buffer, 256, "8%X%X0 - V%X=V%X", x, y, x, y); return buffer;
                case 0x1: snprintf(buffer, 256, "8%X%X1 - V%X|=V%X", x, y, x, y); return buffer;
                case 0x2: snprintf(buffer, 256, "8%X%X2 - V%X&=V%X", x, y, x, y); return buffer;
                case 0x3: snprintf(buffer, 256, "8%X%X3 - V%X^=V%X", x, y, x, y); return buffer;
                case 0x4: snprintf(buffer, 256, "8%X%X4 - V%X+=V%X (carry)", x, y, x, y); return buffer;
                case 0x5: snprintf(buffer, 256, "8%X%X5 - V%X-=V%X (borrow)", x, y, x, y); return buffer;
                case 0x6: snprintf(buffer, 256, "8%X%X6 - V%X>>=1", x, y, x); return buffer;
                case 0x7: snprintf(buffer, 256, "8%X%X7 - V%X=V%X-V%X", x, y, x, y, x); return buffer;
                case 0xE: snprintf(buffer, 256, "8%X%XE - V%X<<=1", x, y, x); return buffer;
                default: return "8XYN - Unknown";
            }
        }
        
        case 0x9000: snprintf(buffer, 256, "9%X%X0 - Skip if V%X!=V%X", x, y, x, y); return buffer;
        case 0xA000: snprintf(buffer, 256, "A%03X - I=0x%03X", nnn, nnn); return buffer;
        case 0xB000: snprintf(buffer, 256, "B%03X - Jump 0x%03X+V0", nnn, nnn); return buffer;
        case 0xC000: snprintf(buffer, 256, "C%X%02X - V%X=rand()&0x%02X", x, nn, x, nn); return buffer;
        case 0xD000: snprintf(buffer, 256, "D%X%X%X - Draw V%X,V%X h=%X", x, y, n, x, y, n); return buffer;
        
        case 0xE000:
            if((opcode & 0xFF) == 0x9E) snprintf(buffer, 256, "E%X9E - Skip if key[V%X]", x, x);
            else snprintf(buffer, 256, "E%XA1 - Skip if !key[V%X]", x, x);
            return buffer;
        
        case 0xF000: {
            switch(nn) {
                case 0x07: snprintf(buffer, 256, "F%X07 - V%X=delay_timer", x, x); return buffer;
                case 0x0A: snprintf(buffer, 256, "F%X0A - V%X=key_wait", x, x); return buffer;
                case 0x15: snprintf(buffer, 256, "F%X15 - delay_timer=V%X", x, x); return buffer;
                case 0x18: snprintf(buffer, 256, "F%X18 - sound_timer=V%X", x, x); return buffer;
                case 0x1E: snprintf(buffer, 256, "F%X1E - I+=V%X", x, x); return buffer;
                case 0x29: snprintf(buffer, 256, "F%X29 - I=sprite(V%X)", x, x); return buffer;
                case 0x33: snprintf(buffer, 256, "F%X33 - BCD V%X@I", x, x); return buffer;
                case 0x55: snprintf(buffer, 256, "F%X55 - Store V0-V%X@I", x, x); return buffer;
                case 0x65: snprintf(buffer, 256, "F%X65 - Load V0-V%X@I", x, x); return buffer;
                default: snprintf(buffer, 256, "F%X%02X - Unknown", x, nn); return buffer;
            }
        }
        
        default: snprintf(buffer, 256, "%04X - Unknown", opcode); return buffer;
    }
}

uint8_t chip8_fontset[80] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

Chip8::Chip8() { initialise(); }

void Chip8::initialise() {
  pc = 0x200;
  opcode = 0;
  index = 0;
  sp = 0;

  memset(display, 0, sizeof(display));
  memset(stack, 0, sizeof(stack));
  memset(v, 0, sizeof(v));
  memset(memory, 0, sizeof(memory));
  memset(key, 0, sizeof(key));

  load_fonts();
  delay_timer = 0;
  sound_timer = 0;
  draw_flag = false;
}

void Chip8::load_fonts() {
  for (int i = 0; i < 80; i++)
    memory[i] = chip8_fontset[i];
}

void Chip8::load_rom(const std::string &filename) {
  std::ifstream file(filename, std::ios::binary | std::ios::ate);

  if (!file.is_open()) {
    std::cerr << "Failed to open ROM: " << filename << std::endl;
    return;
  }

  std::streamsize size = file.tellg();
  file.seekg(0, std::ios::beg);

  if (size > (4096 - 512)) { // 512 reserved for fonts/interpreter
    std::cerr << "ROM too large to fit in memory" << std::endl;
    return;
  }

  file.read((char *)(memory + 512), size);
  file.close();

  std::cout << "Loaded ROM: " << filename << std::endl;
}

void Chip8::emulate_cycle() {
  if (pc >= 4095) return; // Prevent out-of-bounds opcode fetch
  opcode = memory[pc] << 8 | memory[pc + 1]; // 16-bit instruction

  switch (opcode & 0xF000) { // Gets only the first 4 bits
  case 0x0000:
    switch (opcode & 0x00FF) {
    case 0x00E0: // Clears the display
      std::memset(display, 0, sizeof(display));
      draw_flag = true;
      pc += 2;
      break;
    case 0x00EE: // Returns from subroutine
      if (sp > 0) {
        sp--;
        pc = stack[sp];
        pc += 2;
      }
      break;
    default:
      std::cerr << "Unknown opcode: 0x" << std::hex << opcode << std::endl;
      pc += 2;
    }
    break;
  case 0x1000: // 1XXX = Jump to address XXX
    pc = opcode & 0x0FFF;
    break;
  case 0x2000: // 2XXX = Call subroutine at XXX
    if (sp < 16) {
      stack[sp] = pc;
      sp++;
      pc = opcode & 0x0FFF;
    }
    break;
  case 0x3000: // 3XNN = Skip next instruction if v[x] = NN
    if (v[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF))
      pc += 4;
    else
      pc += 2;
    break;
  case 0x4000: // 4XNN - Skip next instruction if v[x] != NN
    if (v[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF))
      pc += 4;
    else
      pc += 2;
    break;
  case 0x5000: // 5XY0 - Skip next instruction if v[x] == v[y]
    if (v[(opcode & 0x0F00) >> 8] == v[(opcode & 0x00F0) >> 4])
      pc += 4;
    else
      pc += 2;
    break;
  case 0x6000: // 6XNN = set v[n] = NN
    v[(opcode & 0x0F00) >> 8] = opcode & 0x00FF;
    pc += 2;
    break;
  case 0x7000: // 7XNN = add NN to v[x]
    v[(opcode & 0x0F00) >> 8] += opcode & 0x00FF;
    pc += 2;
    break;
  case 0x8000:                 // Arithmetic operations
    switch (opcode & 0x000F) { // Look only at last 4 bits
    // Instruction is of the form 8XYN
    case 0x0000: // v[x] = v[y]
      v[(opcode & 0x0F00) >> 8] = v[(opcode & 0x00F0) >> 4];
      pc += 2;
      break;
    case 0x0001: // v[x] = v[x] | v[y]
      v[(opcode & 0x0F00) >> 8] |= v[(opcode & 0x00F0) >> 4];
      pc += 2;
      break;
    case 0x0002: // v[x] = v[y] & v[y]
      v[(opcode & 0x0F00) >> 8] &= v[(opcode & 0x00F0) >> 4];
      pc += 2;
      break;
    case 0x0003: // v[x] = v[y] ^ v[y]
      v[(opcode & 0x0F00) >> 8] ^= v[(opcode & 0x00F0) >> 4];
      pc += 2;
      break;
    case 0x0004: { // v[x] += v[y], v[F] = carry
      uint16_t sum = v[(opcode & 0x0F00) >> 8] + v[(opcode & 0x00F0) >> 4];
      uint8_t flag = (sum > 0xFF) ? 1 : 0;
      v[(opcode & 0x0F00) >> 8] = sum & 0xFF;
      v[0xF] = flag;
      pc += 2;
    } break;
    case 0x0005: { // v[x] -= v[y], v[F] = NOT(borrow)
      uint8_t flag = (v[(opcode & 0x0F00) >> 8] >= v[(opcode & 0x00F0) >> 4]) ? 1 : 0;
      v[(opcode & 0x0F00) >> 8] -= v[(opcode & 0x00F0) >> 4];
      v[0xF] = flag;
      pc += 2;
    } break;
    case 0x0006: { // v[x] >>= 1, v[F] = LSB
      uint8_t flag = v[(opcode & 0x0F00) >> 8] & 0x1;
      v[(opcode & 0x0F00) >> 8] >>= 1;
      v[0xF] = flag;
      pc += 2;
    } break;
    case 0x0007: { // v[x] = v[y] - v[x], v[F] = NOT(borrow)
      uint8_t flag = (v[(opcode & 0x00F0) >> 4] >= v[(opcode & 0x0F00) >> 8]) ? 1 : 0;
      v[(opcode & 0x0F00) >> 8] = v[(opcode & 0x00F0) >> 4] - v[(opcode & 0x0F00) >> 8];
      v[0xF] = flag;
      pc += 2;
    } break;
    case 0x000E: { // v[x] <<= 1, v[F] = MSB
      uint8_t flag = v[(opcode & 0x0F00) >> 8] >> 7; // Save MSB
      v[(opcode & 0x0F00) >> 8] <<= 1;
      v[0xF] = flag;
      pc += 2;
    } break;
    default:
      std::cerr << "Unknown opcode: 0x" << std::hex << opcode << std::endl;
      pc += 2;
      break;
    }
    break;
  case 0x9000: // 9XY0 = Skip next instr. if v[x] != v[y]
    if (v[(opcode & 0x0F00) >> 8] != v[(opcode & 0x00F0) >> 4])
      pc += 4;
    else
      pc += 2;
    break;
  case 0xA000: // AXXX = set index to XXX
    index = opcode & 0x0FFF;
    pc += 2;
    break;
  case 0xB000: // BXXX = jump to address XXX + v[0]
    pc = (opcode & 0xFFF) + v[0];
    break;
  case 0xC000: { // CXNN - v[x] = random_byte & NN
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dist(0, 255);
    v[(opcode & 0x0F00) >> 8] = dist(gen) & (opcode & 0x00FF);
    pc += 2;
  } break;
  case 0xD000: { // DXYN = draw sprite at (v[x], v[y]) with height N
    uint8_t x = v[(opcode & 0x0F00) >> 8], y = v[(opcode & 0x00F0) >> 4];
    uint8_t height = opcode & 0x000F, pixel;

    v[0xF] = 0; // Resetting collision flag
    // Looping through each row of the sprite
    for (int y_line = 0; y_line < height; y_line++) {
      if (index + y_line >= 4096) break;
      pixel = memory[index + y_line]; // One row of sprite data
      // Now looping through each pixel in the row (8)
      for (int x_line = 0; x_line < 8; x_line++) {
        // Check if current pixel is 1
        if ((pixel & (0x80 >> x_line)) != 0) {
          int screen_x = ((x & 63) + x_line) & 63;       // ← WRAPPED
          int screen_y = ((y & 31) + y_line) & 31;       // ← WRAPPED
          int screen_index = screen_x + (screen_y * 64); // 1D display array
          // Checking for collision
          if (display[screen_index] == 1)
            v[0xF] = 1; // Set collision flag
          // We now flip the pixel
          display[screen_index] ^= 1;
        }
      }
    }
    draw_flag = true;
    pc += 2;
  } break;
  case 0xE000:
    switch (opcode & 0x00FF) {
    case 0x009E: // EX9E = skip next instr. if key[v[x]] is pressed
      if (key[v[(opcode & 0x0F00) >> 8]] != 0)
        pc += 4;
      else
        pc += 2;
      break;
    case 0x00A1: // EXA1 = skip next instr. if key[v[x]] is not pressed
      if (key[v[(opcode & 0x0F00) >> 8]] == 0)
        pc += 4;
      else
        pc += 2;
      break;
    default:
      std::cerr << "Unknown opcode: 0x" << std::hex << opcode << std::endl;
      pc += 2;
    }
    break;
  case 0xF000: // Timers and memory operations
    switch (opcode & 0x00FF) {
    case 0x0007: // FX07 - v[x] = delay_timer
      v[(opcode & 0x0F00) >> 8] = delay_timer;
      pc += 2;
      break;
    case 0x000A: { // FX0A - wait for key press, store in v[x]
      bool key_pressed = false;
      for (int i = 0; i < 16; i++) {
        if (key[i] != 0) {
          v[(opcode & 0x0F00) >> 8] = i;
          key_pressed = true;
          break;
        }
      }
      if (key_pressed)
        pc += 2;
    } break;
    case 0x0015: // FX15 - delay_timer = v[x]
      delay_timer = v[(opcode & 0x0F00) >> 8];
      pc += 2;
      break;
    case 0x0018: // FX18 - sound_timer = v[x]
      sound_timer = v[(opcode & 0x0F00) >> 8];
      pc += 2;
      break;
    case 0x001E: // FX1E - index += v[x]
      index += v[(opcode & 0x0F00) >> 8];
      pc += 2;
      break;
    case 0x0029: // FX29 - index = location of sprite for digit v[x]
      index = v[(opcode & 0x0F00) >> 8] * 5;
      pc += 2;
      break;
    case 0x0033: { // FX33 - store BCD representation of v[x] at index
      uint8_t value = v[(opcode & 0x0F00) >> 8];
      if (index < 4096) memory[index] = value / 100;
      if (index + 1 < 4096) memory[index + 1] = value / 10;
      if (index + 2 < 4096) memory[index + 2] = value % 10;
      pc += 2;
    } break;
    case 0x0055: // FX55 - store v[0] to v[x] in memory starting from index
      for (int i = 0; i <= ((opcode & 0x0F00) >> 8); i++) {
        if (index + i < 4096) {
          memory[index + i] = v[i];
        }
      }
      pc += 2;
      break;
    case 0x0065: // FX65 - Fill v[0] to v[x] from memory starting at
                 // index
      for (int i = 0; i <= ((opcode & 0x0F00) >> 8); i++) {
        if (index + i < 4096) {
          v[i] = memory[index + i];
        }
      }
      pc += 2;
      break;
    default:
      std::cerr << "Unknown opcode: 0x" << std::hex << opcode << std::endl;
      pc += 2;
    }
    break;
  default:
    std::cerr << "Unknown opcode: 0x" << std::hex << opcode << std::endl;
    pc += 2;
    break;
  }
}

void Chip8::decrease_delay_timer() {
  if (delay_timer > 0)
    delay_timer--;
}

void Chip8::decrease_sound_timer() {
  if (sound_timer > 0) {
    if (sound_timer == 1)
      std::cout << "BEEP!" << std::endl;
    sound_timer--;
  }
}

bool Chip8::save_state(const std::string& filename, const std::string& current_rom_name) {
    SaveState state;
    
    std::memset(state.rom_name, 0, sizeof(state.rom_name));
    std::strncpy(state.rom_name, current_rom_name.c_str(), sizeof(state.rom_name) - 1);
    state.version = 1;

    std::memcpy(state.memory, memory, sizeof(memory));
    std::memcpy(state.v, v, sizeof(v));
    state.I = index;
    state.PC = pc;
    state.SP = sp;
    std::memcpy(state.stack, stack, sizeof(stack));
    state.delay_timer = delay_timer;
    state.sound_timer = sound_timer;
    std::memcpy(state.display, display, sizeof(display));
    std::memcpy(state.key, key, sizeof(key));
    
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "ERROR: Failed to save state to " << filename << std::endl;
        return false;
    }
    
    file.write((char*)&state, sizeof(SaveState));
    file.close();
    
    std::cout << "State saved to " << filename << std::endl;
    return true;
}

int Chip8::load_state(const std::string& filename, const std::string& current_rom_name) {
    SaveState state;
    
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "ERROR: Failed to load state from " << filename << std::endl;
        return 0; // Not found
    }
    
    file.read((char*)&state, sizeof(SaveState));
    if (!file) {
        std::cerr << "ERROR: File corrupted or wrong size" << std::endl;
        file.close();
        return 0; // Corrupted
    }
    file.close();
    
    if (state.version != 1 || std::string(state.rom_name) != current_rom_name) {
        std::cerr << "ERROR: Save state belongs to another ROM or format." << std::endl;
        return -1; // Wrong ROM
    }
    
    std::memcpy(memory, state.memory, sizeof(memory));
    std::memcpy(v, state.v, sizeof(v));
    index = state.I;
    pc = state.PC;
    sp = state.SP;
    std::memcpy(stack, state.stack, sizeof(stack));
    delay_timer = state.delay_timer;
    sound_timer = state.sound_timer;
    std::memcpy(display, state.display, sizeof(display));
    std::memcpy(key, state.key, sizeof(key));
    
    std::cout << "State loaded from " << filename << std::endl;
    return 1; // Success
}