#define  SDL_DISABLE_IMMINTRIN_H 1
#include <SDL2/SDL.h>

#include "asm.h"
#include "renderer.h"


void setup_graphics (Render_Context *context, int width, int height, int emulated_width, int emulated_height) {
#ifndef DEBUG
  SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO);
  
  context->window = SDL_CreateWindow("EKAGGATA",
                                     SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                     width, height, 0);
  
  context->renderer = SDL_CreateRenderer(context->window, -1,
                                         SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
  
  context->display = SDL_CreateTexture(context->renderer, SDL_PIXELFORMAT_RGBA8888,
                                       SDL_TEXTUREACCESS_STREAMING,
                                       emulated_width, emulated_height);

  context->emulated_width  = emulated_width;
  context->emulated_height = emulated_height;
#endif

}

void render (Render_Context *context, CPU *cpu) {
#ifndef DEBUG
  // Copy texture to render target
  SDL_UpdateTexture(context->display, NULL, cpu->framebuffer,
                    context->emulated_width*sizeof(uint32_t)); // @Merk - ganger her med bredden av skjermen (bytePitch)
    
  SDL_SetRenderTarget(context->renderer, NULL);
        
  //Clear screen
  SDL_RenderClear(context->renderer);

  SDL_Rect framebuffer_source_dimensions = {
    .x = 0,
    .y = 0,
    .w = context->emulated_width,
    .h = context->emulated_width,
  };
    
  //Render texture to screen
  SDL_RenderCopy(context->renderer, context->display, &framebuffer_source_dimensions, NULL); // NULL viser til at skjermen skal fylles med hvanåenn var i source_rect
    
  //Update screen
  SDL_RenderPresent(context->renderer);
#endif
}


void renderer_cleanup (Render_Context *context) {
#ifndef DEBUG
  SDL_DestroyTexture(context->display);
  SDL_DestroyRenderer(context->renderer);
  SDL_DestroyWindow(context->window);
  SDL_Quit();
#endif
}
