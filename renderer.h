#ifndef RENDERER_H
#define RENDERER_H

#include <SDL2/SDL.h>
#include "base.h"

typedef struct Render_Context {
  SDL_Window   *window;
  SDL_Renderer *renderer;
  SDL_Texture  *display;

  int emulated_width;
  int emulated_height;
} Render_Context;

void setup_graphics   (Render_Context *context, int width, int height, int emulated_width, int emulated_height);
void render           (Render_Context *context, CPU *cpu);
void renderer_cleanup (Render_Context *context);

#endif
