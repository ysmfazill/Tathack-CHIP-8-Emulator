#include "chip8.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include <SDL2/SDL_error.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_stdinc.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_video.h>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <cmath>
#include <filesystem>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifdef _WIN32
#include <windows.h>
#endif

std::string get_rom_name(const std::string& path) {
    size_t last_slash = path.find_last_of("/\\");
    std::string name = (last_slash == std::string::npos) ? path : path.substr(last_slash + 1);
    size_t last_dot = name.find_last_of('.');
    if (last_dot != std::string::npos) name = name.substr(0, last_dot);
    std::transform(name.begin(), name.end(), name.begin(), ::toupper);
    return name;
}

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"

const int SCALE = 10;
const int PADDING = 20;
const int GAME_WIDTH = 64 * SCALE;
const int GAME_HEIGHT = 32 * SCALE;
const int PANEL_WIDTH = 300;
const int DEBUG_WIDTH = 250;
const int WIDTH = GAME_WIDTH + PANEL_WIDTH + DEBUG_WIDTH + (PADDING * 4);
const int HEIGHT = GAME_HEIGHT + (PADDING * 2);

uint8_t keymap[16] = {
    SDLK_x, SDLK_1, SDLK_2, SDLK_3,
    SDLK_q, SDLK_w, SDLK_e, SDLK_a,
    SDLK_s, SDLK_d, SDLK_z, SDLK_c,
    SDLK_4, SDLK_r, SDLK_f, SDLK_v
};

struct AudioState {
    bool beeping = false;
    bool test_beeping = false;
    int test_timer_ms = 0;
    uint32_t last_test_ticks = 0;
    
    int waveform = 0; // 0=Square, 1=Sine, 2=Triangle, 3=Sawtooth
    int frequency = 440;
    int volume = 50;
    uint32_t sample_index = 0;
};

void audio_callback(void *userdata, uint8_t *stream, int len) {
    AudioState* state = (AudioState*)userdata;
    int16_t* audio_buffer = (int16_t*)stream;
    int samples = len / 2;
    const float SAMPLE_RATE = 44100.0f;
    const float MAX_AMPLITUDE = 32767.0f;

    for (int i = 0; i < samples; i++) {
        if (state->beeping || state->test_beeping) {
            float time = state->sample_index / SAMPLE_RATE;
            float period = 1.0f / state->frequency;
            float phase = fmod(time, period) / period; // 0.0 to 1.0

            float amplitude = (state->volume / 100.0f) * MAX_AMPLITUDE;
            int16_t value = 0;

            if (state->waveform == 0) { // Square
                value = (phase < 0.5f) ? amplitude : -amplitude;
            } else if (state->waveform == 1) { // Sine
                value = amplitude * sin(2.0f * M_PI * state->frequency * time);
            } else if (state->waveform == 2) { // Triangle
                float val = 4.0f * fabs(phase - 0.5f) - 1.0f;
                value = amplitude * val;
            } else if (state->waveform == 3) { // Sawtooth
                float val = 2.0f * phase - 1.0f;
                value = amplitude * val;
            }

            audio_buffer[i] = value;
            state->sample_index++;
        } else {
            audio_buffer[i] = 0;
            state->sample_index = 0;
        }
    }
}

void draw_graphics(SDL_Renderer *renderer, Chip8 &chip8, int color_theme, bool enable_ghost, SaveState* ghost_state, bool enable_grid) {
  if (color_theme == 2) { // Amber
    SDL_SetRenderDrawColor(renderer, 15, 10, 0, 255);
  } else if (color_theme == 3) { // Neon
    SDL_SetRenderDrawColor(renderer, 0, 15, 0, 255);
  } else if (color_theme == 4) { // High Contrast
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  } else { // Classic
    SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
  }
  SDL_RenderClear(renderer);
  
  if (color_theme == 2) { // Amber
    SDL_SetRenderDrawColor(renderer, 255, 176, 0, 255);
  } else if (color_theme == 3) { // Neon
    SDL_SetRenderDrawColor(renderer, 0, 255, 128, 255);
  } else if (color_theme == 4) { // High Contrast
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
  } else { // Classic
    SDL_SetRenderDrawColor(renderer, 240, 240, 240, 255);
  }

  for (int y = 0; y < 32; y++) {
    for (int x = 0; x < 64; x++) {
      if (chip8.display[x + (y * 64)] == 1) {
        int render_scale = enable_grid ? (SCALE - 1) : SCALE;
        SDL_Rect rect = {(x * SCALE) + PADDING, (y * SCALE) + PADDING, render_scale, render_scale};
        SDL_RenderFillRect(renderer, &rect);
      }
    }
  }

  if (enable_ghost && ghost_state) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    if (color_theme == 2) {
      SDL_SetRenderDrawColor(renderer, 255, 176, 0, 64);
    } else if (color_theme == 3) {
      SDL_SetRenderDrawColor(renderer, 0, 255, 128, 64);
    } else if (color_theme == 4) {
      SDL_SetRenderDrawColor(renderer, 255, 255, 255, 64);
    } else {
      SDL_SetRenderDrawColor(renderer, 240, 240, 240, 64);
    }

    for (int y = 0; y < 32; y++) {
      for (int x = 0; x < 64; x++) {
        if (ghost_state->display[x + (y * 64)] == 1) {
          int render_scale = enable_grid ? (SCALE - 1) : SCALE;
          SDL_Rect rect = {(x * SCALE) + PADDING, (y * SCALE) + PADDING, render_scale, render_scale};
          SDL_RenderFillRect(renderer, &rect);
        }
      }
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
  }
}

void reload_ghost_state(SaveState& ghost_state, bool& ghost_valid, const std::string& filename, const std::string& current_rom_name) {
    std::ifstream file(filename, std::ios::binary);
    if (file.is_open()) {
        file.read((char*)&ghost_state, sizeof(SaveState));
        if (file.good() && ghost_state.version == 1 && std::string(ghost_state.rom_name) == current_rom_name) {
            ghost_valid = true;
        } else {
            ghost_valid = false;
        }
        file.close();
    } else {
        ghost_valid = false;
    }
}

void handle_input(Chip8 &chip8, bool &running, int &cycles_per_frame, int &color_theme, SaveState& ghost_state, bool& ghost_valid, const std::string& save_path, const std::string& rom_name, bool& show_load_error, bool& blinky_game_over, int& blinky_score, bool& blinky_collision_latch, const std::string& rom_file) {
  SDL_Event event;

  while (SDL_PollEvent(&event)) {
    ImGui_ImplSDL2_ProcessEvent(&event);
    if (event.type == SDL_QUIT)
      running = false;
    
    if (ImGui::GetIO().WantCaptureKeyboard) continue;

    if (event.type == SDL_KEYDOWN) {
      if (event.key.keysym.sym == SDLK_ESCAPE) {
        running = false;
      }
      else if (event.key.keysym.sym == SDLK_r || event.key.keysym.sym == SDLK_RETURN) {
        if (rom_name == "BLINKY" && blinky_game_over) {
            blinky_score = 0;
            blinky_game_over = false;
            blinky_collision_latch = false;
            chip8.load_rom(rom_file);
        }
      }
      else if (event.key.keysym.sym == SDLK_UP) {
          cycles_per_frame++;
          if (cycles_per_frame > 30) cycles_per_frame = 30;
          std::cout << "Speed increased: " << cycles_per_frame << " cycles/frame\n";
      }
      else if (event.key.keysym.sym == SDLK_DOWN) {
          cycles_per_frame--;
          if (cycles_per_frame < 1) cycles_per_frame = 1;
          std::cout << "Speed decreased: " << cycles_per_frame << " cycles/frame\n";
      }
      else if (event.key.keysym.sym == SDLK_F5) {
          chip8.save_state(save_path, rom_name);
          reload_ghost_state(ghost_state, ghost_valid, save_path, rom_name);
      }
      else if (event.key.keysym.sym == SDLK_F9) {
          if (!chip8.load_state(save_path, rom_name)) {
              show_load_error = true;
          }
      }
      else if (event.key.keysym.sym == SDLK_F1) color_theme = 1;
      else if (event.key.keysym.sym == SDLK_F2) color_theme = 2;
      else if (event.key.keysym.sym == SDLK_F3) color_theme = 3;
      else if (event.key.keysym.sym == SDLK_F4) color_theme = 4;
      
      for (int i = 0; i < 16; i++) {
        if (event.key.keysym.sym == keymap[i])
          chip8.key[i] = 1;
      }
    }
    if (event.type == SDL_KEYUP) {
      for (int i = 0; i < 16; i++) {
        if (event.key.keysym.sym == keymap[i])
          chip8.key[i] = 0;
      }
    }
  }
}

int main(int argc, char **argv) {
#ifdef _WIN32
  HWND hwnd = GetConsoleWindow();
  if (hwnd) ShowWindow(hwnd, SW_HIDE);
#endif

  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <ROM file>" << std::endl;
    return 1;
  }
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) < 0) {
    std::cerr << "SDL Error: " << SDL_GetError() << std::endl;
    return 1;
  }
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0"); // Ensure nearest-neighbor scaling
  
  AudioState audio_state;
  SDL_AudioSpec want, have;
  SDL_zero(want);
  want.freq = 44100;
  want.format = AUDIO_S16SYS;
  want.channels = 1;
  want.samples = 2048;
  want.callback = audio_callback;
  want.userdata = &audio_state;

  SDL_AudioDeviceID audio_device = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
  if (audio_device != 0) SDL_PauseAudioDevice(audio_device, 0);

  SDL_Window *window = SDL_CreateWindow("Chip-8 Emulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
  if (!window) return 1;

  SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (!renderer) return 1;

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO(); (void)io;
  io.IniFilename = nullptr; // Disable imgui.ini to strictly enforce our fixed layout
  ImGui::StyleColorsDark();

  ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
  ImGui_ImplSDLRenderer2_Init(renderer);

  Chip8 chip8;
  chip8.load_rom(argv[1]);

  bool running = true;
  const uint32_t TARGET_FRAME_TIME = 16;
  int cycles_per_frame = 10;
  int color_theme = 4; // Default to High Contrast (Pure B&W) for maximum generic legibility

  SaveState ghost_state;
  bool ghost_valid = false;
  bool enable_ghost = false;

  uint32_t last_ticks = SDL_GetTicks();
  float timer_accumulator = 0.0f;

  std::string rom_name = get_rom_name(argv[1]);
  std::filesystem::create_directory("savestates");
  std::string save_path = "savestates/" + rom_name + ".sav";
  
  bool enable_grid = false;
  bool show_load_error = false;
  float error_timer = 0.0f;
  
  bool blinky_assist = false;
  int blinky_score = 0;
  bool blinky_caught = false;
  float blinky_caught_timer = 0.0f;
  bool blinky_collision_latch = false;
  bool blinky_game_over = false;

  while (running) {
    uint32_t frame_start = SDL_GetTicks();
    uint32_t dt = frame_start - last_ticks;
    last_ticks = frame_start;
    timer_accumulator += dt;

    if (show_load_error) {
        error_timer += dt;
        if (error_timer > 3000.0f) {
            show_load_error = false;
            error_timer = 0.0f;
        }
    }

    handle_input(chip8, running, cycles_per_frame, color_theme, ghost_state, ghost_valid, save_path, rom_name, show_load_error, blinky_game_over, blinky_score, blinky_collision_latch, argv[1]);
    
    for (int i = 0; i < cycles_per_frame; i++) {
      chip8.emulate_cycle();
      if (chip8.draw_flag) {
        chip8.draw_flag = false;
        break; // Yield on draw to prevent flickering/invisible sprites
      }
    }

    if (rom_name == "BLINKY" && blinky_assist && !blinky_game_over) {
        int px = chip8.v[8];
        int py = chip8.v[9];
        int g1x = chip8.v[0xA];
        int g1y = chip8.v[0xB];
        int g2x = chip8.v[0xC];
        int g2y = chip8.v[0xD];

        bool collision = false;
        if (std::abs(px - g1x) < 4 && std::abs(py - g1y) < 4) collision = true;
        if (std::abs(px - g2x) < 4 && std::abs(py - g2y) < 4) collision = true;

        if (collision) {
            if (!blinky_collision_latch) {
                blinky_score += 100;
                blinky_caught = true;
                blinky_caught_timer = 750.0f; // 750ms
                blinky_collision_latch = true;
                
                // Reset Pac-Man / Item to start position
                chip8.v[8] = 0x1A;
                chip8.v[9] = 0x0C;
            }
        } else {
            blinky_collision_latch = false;
        }

        if (blinky_caught) {
            blinky_caught_timer -= dt;
            if (blinky_caught_timer <= 0.0f) {
                blinky_caught = false;
                blinky_game_over = true;
            }
        }
    }

    while (timer_accumulator >= (1000.0f / 60.0f)) {
      chip8.decrease_delay_timer();
      chip8.decrease_sound_timer();
      timer_accumulator -= (1000.0f / 60.0f);
    }

    uint32_t current_ticks = SDL_GetTicks();
    if (audio_state.test_timer_ms > 0) {
        audio_state.test_timer_ms -= (current_ticks - audio_state.last_test_ticks);
        if (audio_state.test_timer_ms <= 0) {
            audio_state.test_timer_ms = 0;
            audio_state.test_beeping = false;
        }
    }
    audio_state.last_test_ticks = current_ticks;
    audio_state.beeping = (chip8.get_sound_timer() > 0);

    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    // Position the UI panel strictly to the right of the padded CHIP-8 logical display
    ImGui::SetNextWindowPos(ImVec2(GAME_WIDTH + (PADDING * 2), PADDING), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(PANEL_WIDTH, HEIGHT - (PADDING * 2)), ImGuiCond_Always);

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
    ImGui::Begin("Features Showcase", NULL, window_flags);
    ImGui::Text("ROM: %s", rom_name.c_str());
    ImGui::Separator();
    
    ImGui::SliderInt("Speed", &cycles_per_frame, 1, 30);
    ImGui::Checkbox("Pixel Grid (Helps Tetris)", &enable_grid);
    
    if (ImGui::Button("Save State")) {
        chip8.save_state(save_path, rom_name);
        reload_ghost_state(ghost_state, ghost_valid, save_path, rom_name);
    }
    ImGui::SameLine();
    if (ImGui::Button("Load State")) {
        if (!chip8.load_state(save_path, rom_name)) {
            show_load_error = true;
            error_timer = 0.0f;
        }
    }

    if (show_load_error) {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Save state belongs to another ROM.");
    }
    
    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Save State: %s.sav", rom_name.c_str());

    if (ImGui::Checkbox("Save State Ghost", &enable_ghost)) {
        if (enable_ghost) {
            reload_ghost_state(ghost_state, ghost_valid, save_path, rom_name);
        }
    }

    if (rom_name == "BLINKY") {
        ImGui::Separator();
        ImGui::Checkbox("Blinky Game Assist", &blinky_assist);
        if (blinky_assist) {
            ImGui::Text("Blinky Score: %06d", blinky_score);
            if (ImGui::Button("Reset Score")) {
                blinky_score = 0;
            }
        }
    }

    ImGui::Text("Color Palette");
    ImGui::RadioButton("Classic (White/Black)", &color_theme, 1);
    ImGui::RadioButton("Retro (Amber)", &color_theme, 2);
    ImGui::RadioButton("Matrix (Neon)", &color_theme, 3);
    ImGui::RadioButton("High Contrast (Pure B&W)", &color_theme, 4);

    ImGui::Separator();
    if (ImGui::CollapsingHeader("AUDIO SETTINGS", ImGuiTreeNodeFlags_DefaultOpen)) {
        const char* waveforms[] = { "Square", "Sine", "Triangle", "Sawtooth" };
        ImGui::Combo("Waveform", &audio_state.waveform, waveforms, IM_ARRAYSIZE(waveforms));
        ImGui::SliderInt("Frequency", &audio_state.frequency, 100, 2000, "%d Hz");
        ImGui::SliderInt("Volume", &audio_state.volume, 0, 100, "%d%%");
        if (ImGui::Button("TEST SOUND")) {
            audio_state.test_timer_ms = 200; // 200ms duration
            audio_state.test_beeping = true;
            audio_state.last_test_ticks = SDL_GetTicks();
        }
    }
    
    ImGui::End();

    // CPU Inspector Panel
    ImGui::SetNextWindowPos(ImVec2(GAME_WIDTH + PANEL_WIDTH + (PADDING * 3), PADDING), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(DEBUG_WIDTH, HEIGHT - (PADDING * 2)), ImGuiCond_Always);
    ImGui::Begin("CPU Inspector", NULL, window_flags);
    ImGui::Text("Display: 64 x 32");
    ImGui::Text("CPU Speed: %d cycles/s", cycles_per_frame * 60);
    ImGui::Separator();
    ImGui::Text("Opcode: 0x%04X", chip8.opcode);
    ImGui::Text("PC: 0x%03X   I: 0x%03X", chip8.pc, chip8.index);
    ImGui::Text("SP: 0x%02X", chip8.sp);
    ImGui::Text("Delay: %02d    Sound: %02d", chip8.delay_timer, chip8.sound_timer);
    ImGui::Separator();
    ImGui::Text("Registers:");
    for (int i = 0; i < 8; i++) {
        ImGui::Text("V%X: %02X     V%X: %02X", i, chip8.v[i], i+8, chip8.v[i+8]);
    }
    ImGui::End();

    if (rom_name == "BLINKY" && blinky_assist && blinky_caught) {
        ImGui::SetNextWindowPos(ImVec2(GAME_WIDTH / 2.0f + PADDING, GAME_HEIGHT / 2.0f + PADDING), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::Begin("Overlay", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "     CAUGHT!     ");
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "      +100       ");
        ImGui::End();
    }
    
    if (rom_name == "BLINKY" && blinky_assist && blinky_game_over) {
        ImGui::SetNextWindowPos(ImVec2(GAME_WIDTH / 2.0f + PADDING, GAME_HEIGHT / 2.0f + PADDING), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::Begin("Game Over", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "========================");
        ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "       GAME OVER        ");
        ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "                        ");
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "     SCORE: %06d      ", blinky_score);
        ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "                        ");
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "     [ R ] RESTART      ");
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "   [ ENTER ] RESTART    ");
        ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "                        ");
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "      [ ESC ] QUIT      ");
        ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "========================");
        ImGui::End();
    }

    draw_graphics(renderer, chip8, color_theme, enable_ghost, ghost_valid ? &ghost_state : nullptr, enable_grid);

    ImGui::Render();
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);

    SDL_RenderPresent(renderer);

    uint32_t elapsed_time = SDL_GetTicks() - frame_start;
    if (elapsed_time < TARGET_FRAME_TIME) {
      SDL_Delay(TARGET_FRAME_TIME - elapsed_time);
    }
  }

  ImGui_ImplSDLRenderer2_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();

  if (audio_device != 0) SDL_CloseAudioDevice(audio_device);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}