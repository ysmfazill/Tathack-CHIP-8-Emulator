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
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"

const int SCALE = 10;
const int WIDTH = 64 * SCALE;
const int HEIGHT = 32 * SCALE;

uint8_t keymap[16] = {
    SDLK_x, SDLK_1, SDLK_2, SDLK_3,
    SDLK_q, SDLK_w, SDLK_e, SDLK_a,
    SDLK_s, SDLK_d, SDLK_z, SDLK_c,
    SDLK_4, SDLK_r, SDLK_f, SDLK_v
};

void audio_callback(void *userdata, uint8_t *stream, int len) {
  static uint32_t sample_index = 0;
  int16_t *audio_buffer = (int16_t *)stream;
  int samples = len / 2;

  bool *beeping = (bool *)userdata;
  for (int i = 0; i < samples; i++) {
    if (*beeping) {
      int16_t value = ((sample_index++ / 100) % 2) ? 3000 : -3000;
      audio_buffer[i] = value;
    } else {
      audio_buffer[i] = 0;
      sample_index = 0;
    }
  }
}

void draw_graphics(SDL_Renderer *renderer, Chip8 &chip8, int color_theme, bool enable_ghost, SaveState* ghost_state) {
  if (color_theme == 2) {
    SDL_SetRenderDrawColor(renderer, 43, 27, 4, 255);
  } else if (color_theme == 3) {
    SDL_SetRenderDrawColor(renderer, 0, 30, 0, 255);
  } else {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  }
  SDL_RenderClear(renderer);
  
  if (color_theme == 2) {
    SDL_SetRenderDrawColor(renderer, 255, 176, 0, 255);
  } else if (color_theme == 3) {
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
  } else {
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
  }

  for (int y = 0; y < 32; y++) {
    for (int x = 0; x < 64; x++) {
      if (chip8.display[x + (y * 64)] == 1) {
        SDL_Rect rect = {x * SCALE, y * SCALE, SCALE, SCALE};
        SDL_RenderFillRect(renderer, &rect);
      }
    }
  }

  if (enable_ghost && ghost_state) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    if (color_theme == 2) {
      SDL_SetRenderDrawColor(renderer, 255, 176, 0, 64);
    } else if (color_theme == 3) {
      SDL_SetRenderDrawColor(renderer, 0, 255, 0, 64);
    } else {
      SDL_SetRenderDrawColor(renderer, 255, 255, 255, 64);
    }

    for (int y = 0; y < 32; y++) {
      for (int x = 0; x < 64; x++) {
        if (ghost_state->display[x + (y * 64)] == 1) {
          SDL_Rect rect = {x * SCALE, y * SCALE, SCALE, SCALE};
          SDL_RenderFillRect(renderer, &rect);
        }
      }
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
  }
}

void reload_ghost_state(SaveState& ghost_state, bool& ghost_valid) {
    std::ifstream file("savestate.bin", std::ios::binary);
    if (file.is_open()) {
        file.read((char*)&ghost_state, sizeof(SaveState));
        ghost_valid = file.good();
        file.close();
    } else {
        ghost_valid = false;
    }
}

void handle_input(Chip8 &chip8, bool &running, int &cycles_per_frame, int &color_theme, SaveState& ghost_state, bool& ghost_valid) {
  SDL_Event event;

  while (SDL_PollEvent(&event)) {
    ImGui_ImplSDL2_ProcessEvent(&event);
    if (event.type == SDL_QUIT)
      running = false;
    
    if (ImGui::GetIO().WantCaptureKeyboard) continue;

    if (event.type == SDL_KEYDOWN) {
      if (event.key.keysym.sym == SDLK_ESCAPE)
        running = false;
      else if (event.key.keysym.sym == SDLK_UP) {
          cycles_per_frame++;
          if (cycles_per_frame > 30) cycles_per_frame = 30;
      }
      else if (event.key.keysym.sym == SDLK_DOWN) {
          cycles_per_frame--;
          if (cycles_per_frame < 1) cycles_per_frame = 1;
      }
      else if (event.key.keysym.sym == SDLK_F5) {
          chip8.save_state("savestate.bin");
          reload_ghost_state(ghost_state, ghost_valid);
      }
      else if (event.key.keysym.sym == SDLK_F9) {
          chip8.load_state("savestate.bin");
      }
      else if (event.key.keysym.sym == SDLK_F1) color_theme = 1;
      else if (event.key.keysym.sym == SDLK_F2) color_theme = 2;
      else if (event.key.keysym.sym == SDLK_F3) color_theme = 3;
      
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
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <ROM file>" << std::endl;
    return 1;
  }
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) < 0) {
    std::cerr << "SDL Error: " << SDL_GetError() << std::endl;
    return 1;
  }
  
  bool beeping = false;
  SDL_AudioSpec want, have;
  SDL_zero(want);
  want.freq = 44100;
  want.format = AUDIO_S16SYS;
  want.channels = 1;
  want.samples = 2048;
  want.callback = audio_callback;
  want.userdata = &beeping;

  SDL_AudioDeviceID audio_device = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
  if (audio_device != 0) SDL_PauseAudioDevice(audio_device, 0);

  SDL_Window *window = SDL_CreateWindow("Chip-8 Emulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
  if (!window) return 1;

  SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (!renderer) return 1;

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO(); (void)io;
  ImGui::StyleColorsDark();

  ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
  ImGui_ImplSDLRenderer2_Init(renderer);

  Chip8 chip8;
  chip8.load_rom(argv[1]);

  bool running = true;
  const uint32_t TARGET_FRAME_TIME = 16;
  int cycles_per_frame = 10;
  int color_theme = 1;

  SaveState ghost_state;
  bool ghost_valid = false;
  bool enable_ghost = false;

  while (running) {
    uint32_t frame_start = SDL_GetTicks();

    handle_input(chip8, running, cycles_per_frame, color_theme, ghost_state, ghost_valid);
    for (int i = 0; i < cycles_per_frame; i++) {
      chip8.emulate_cycle();
    }

    chip8.decrease_delay_timer();
    chip8.decrease_sound_timer();

    beeping = (chip8.get_sound_timer() > 0);

    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Features Showcase");
    ImGui::SliderInt("Speed", &cycles_per_frame, 1, 30);
    
    if (ImGui::Button("Save State")) {
        chip8.save_state("savestate.bin");
        reload_ghost_state(ghost_state, ghost_valid);
    }
    ImGui::SameLine();
    if (ImGui::Button("Load State")) {
        chip8.load_state("savestate.bin");
    }

    if (ImGui::Checkbox("Save State Ghost", &enable_ghost)) {
        if (enable_ghost) {
            reload_ghost_state(ghost_state, ghost_valid);
        }
    }

    ImGui::Text("Color Palette");
    ImGui::RadioButton("Classic (White/Black)", &color_theme, 1);
    ImGui::RadioButton("Retro (Amber)", &color_theme, 2);
    ImGui::RadioButton("Matrix (Neon)", &color_theme, 3);
    ImGui::End();

    draw_graphics(renderer, chip8, color_theme, enable_ghost, ghost_valid ? &ghost_state : nullptr);

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