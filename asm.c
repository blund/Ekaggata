#include <stdint.h>
#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>

#include <limits.h>

#include <unistd.h>
#include "asm.h"

#define  SDL_DISABLE_IMMINTRIN_H 1
#include <SDL2/SDL.h>


// Om grafikk
// https://gamedev.stackexchange.com/questions/157604/how-to-get-access-to-framebuffer-as-a-uint32-t-in-sdl2
// Det neste vi vil gjøre er å bruke en framebuffer til å displaye noe bilder :)
// Kan sikkert gjøres med ren opengl?
// http://www.songho.ca/opengl/gl_pbo.html

// Om lyd:
// https://discourse.libsdl.org/t/feeding-the-audio-buffer/11432/3

// Om hvordan en CPU virker:
// https://everything2.com/title/modifying+IP%252FPC+instead+of+using+%2522JMP%2522



#define STORAGE_OFFSET     4096
#define FRAMEBUFFER_OFFSET 8192

#define DISPLAY_SIZE 128
  

#define INSTR(PC) inst_mem[cpu.r[PC]]
#define ASM(opcode, a, b) (Instr){.op = opcode, .reg_to = a, .reg_from = b}

SDL_Renderer *renderer;
SDL_Texture  *display;

int frames_drawn = 0;

// https://en.wikipedia.org/wiki/X_Macro
const char* register_names[] = {
#define X(name) [name] = #name,
#include "registers.x"
#undef X
};

const char* opcode_names[] = {
#define X(op, name, type) [op] = #name,
#include "opcodes.x"
#undef X
};

const Opcode_Type opcode_types[] = {
#define X(op, name, type) [op] = type,
#include "opcodes.x"
#undef X
};

const Op opcodes[] = {
#define X(op, name, type) [op] = op,
#include "opcodes.x"
#undef X
};



void print_state (CPU* cpu) {
  for (int i = 0; i < 15; i++) {
    printf("R%i = 0x%x (%i)\n", i, cpu->r[i], cpu->r[i]);
  }
  printf("PC = 0x%x (%i)\n", cpu->r[PC],  cpu->r[PC]);
  printf("\n");
}


void print_instr (Instr instr) {
  if (!opcode_types[instr.op]) {
    printf(" -> NOP\n");
    return;
  }
  
  if ((opcodes[instr.op] >= JMP) && (opcodes[instr.op] <= JGT)) {
    printf(" -> JMP %i\n", instr.reg_to);
    return;
  }
    
  printf(" -> %s %s ", opcode_names[instr.op], register_names[instr.reg_to]);
  
  switch (opcode_types[instr.op]) {
  case REG:
    printf("%s\n", register_names[instr.reg_from]);
    break;
  case IMM:
    printf("$0x%x (%i)\n", instr.imm, instr.imm);
    break;
  case ADR:
    printf("[%s]\n", register_names[instr.reg_from]);
    break;
  default:
    printf("Fikk ugyldig opcode-type. Du har nok rørt bs-data\n");
    exit(1);
  }
}

void eval (CPU* cpu, Instr instr);
inline void eval (CPU* cpu, Instr instr) {
  // @TODO - her er det mulighet for å legge inn noen asserts i kjøringen! Som at man har overtrådt minnegrenser eller noe slik.
  switch (instr.op) {

  case MOV:
    cpu->r[instr.reg_to] = cpu->r[instr.reg_from];
    break;

  case MOV_imm:
    
    cpu->r[instr.reg_to] = instr.imm;
    break;

  case ADD:
    cpu->r[instr.reg_to] += cpu->r[instr.reg_from];
    break;

  case ADD_imm:
    cpu->r[instr.reg_to] += instr.imm;
    break;

  case SUB:
    cpu->r[instr.reg_to] -= cpu->r[instr.reg_from];
    break;
    
  case SUB_imm:
    cpu->r[instr.reg_to] -= instr.imm;
    break;

  case MUL:
    cpu->r[instr.reg_to] *= cpu->r[instr.reg_from];
    break;

  case MUL_imm:
    cpu->r[instr.reg_to] *= instr.imm;
    break;

  case DIV:
    cpu->r[instr.reg_to] /= cpu->r[instr.reg_from];
    break;

  case DIV_imm:
    cpu->r[instr.reg_to] /= instr.imm;
    break;

  case LDR_adr:
    cpu->r[instr.reg_to] = *(s32 *)(cpu->memory + cpu->r[instr.reg_from]);
    break;

  case STR_adr:
    *(s32 *)(cpu->memory + cpu->r[instr.reg_from]) = cpu->r[instr.reg_to]; // @MERK at vi bruker reg_to (første argument) som kilde.
    break;
  
  case JMP:
    cpu->r[PC] = instr.reg_to;
    break;
    
  case JMP_imm:
    cpu->r[PC] = instr.reg_to; // @MERK!!!! - for rekkefølge, vil bruke første arg.
    break;

  case JEQ:
    if (cpu->flags & Z) {
      cpu->r[PC] = instr.reg_to;
    } else {
      cpu->r[PC]++;
    }
    break;

  case JNE:
    if (!(cpu->flags & Z)) {
      cpu->r[PC] = instr.reg_to;
    } else {
      cpu->r[PC]++;
    }
    break;

  case JGT:
    if (!(cpu->flags & Z) && (!!(cpu->flags & N) == !!(cpu->flags & V))) {
      cpu->r[PC] = instr.reg_to;
    } else {
      cpu->r[PC]++;
    }
    break;
    
  case JGE:
    if (!!(cpu->flags & N) == !!(cpu->flags & V)) {
      cpu->r[PC] = instr.reg_to;
    } else {
      cpu->r[PC]++;
    }
    break; 

  case JLT:
    if ((cpu->flags & N) != (cpu->flags & V)) {
      cpu->r[PC] = instr.reg_to;
    } else {
      cpu->r[PC]++;
    }
    break;

  case JLE:
    if ((cpu->flags & Z) || (!!(cpu->flags & N) != !!(cpu->flags & V))) {
      cpu->r[PC] = instr.reg_to;
    } else {
      cpu->r[PC]++;
    }
    break;
          
    
  case CMP:
    {
      // Om hvordan man lager LT, GT osv... fra NZCV
      // https://community.arm.com/developer/ip-products/processors/b/processors-ip-blog/posts/condition-codes-1-condition-flags-and-codes
      
      s32 diff              =  cpu->r[instr.reg_to] - cpu->r[instr.reg_from];
      u32 unsigned_overflow = (cpu->r[instr.reg_to] + cpu->r[instr.reg_from]) < cpu->r[instr.reg_to];
      u32 signed_overflow   = (cpu->r[instr.reg_to] < 0) && (cpu->r[instr.reg_from] > (INT_MAX - cpu->r[instr.reg_from]));

      cpu->flags = 0;
      
      cpu->flags |= diff               ? 0 : Z;
      cpu->flags |= diff < 0           ? N : 0;
      cpu->flags |= unsigned_overflow  ? C : 0; 
      cpu->flags |= signed_overflow    ? V : 0;

#ifdef DEBUG
      printf("flags: %x\n", cpu->flags);
      printf("eq: %i\n", (cpu->flags & Z));
      printf("ge: %i\n", !!(cpu->flags & N) == !!(cpu->flags & V));
      printf("gt: %i\n", !(cpu->flags & Z) && (cpu->flags & N) == (cpu->flags & V));
      printf("lt: %i\n", (cpu->flags & N) != (cpu->flags & V));
      #endif
    }
    break;
    
  case CMP_imm:
    {
      // Om hvordan man lager LT, GT osv... fra NZCV
      // https://community.arm.com/developer/ip-products/processors/b/processors-ip-blog/posts/condition-codes-1-condition-flags-and-codes
      
      s32 diff              =  cpu->r[instr.reg_to] - instr.imm;
      u32 unsigned_overflow = (cpu->r[instr.reg_to] + instr.imm) < cpu->r[instr.reg_to];
      s32 signed_overflow   = (cpu->r[instr.reg_to] < 0) && (instr.imm > INT_MAX - instr.imm);

      cpu->flags = 0;
      
      cpu->flags |= diff               ? 0 : Z;
      cpu->flags |= diff < 0           ? N : 0;
      cpu->flags |= unsigned_overflow  ? C : 0; 
      cpu->flags |= signed_overflow    ? V : 0;

      #ifdef DEBUG
      printf("flags: %x\n", cpu->flags);
      printf("eq: %i\n", (cpu->flags & Z));
      printf("ge: %i\n", !!(cpu->flags & N) == !!(cpu->flags & V));
      printf("gt: %i\n", !(cpu->flags & Z) && (cpu->flags & N) == (cpu->flags & V));
      printf("lt: %i\n", (cpu->flags & N) != (cpu->flags & V));
      #endif
    }
    break;

  case DRW:
#ifndef DEBUG
    // Copy texture to render target
    SDL_UpdateTexture(display, NULL, cpu->framebuffer, DISPLAY_SIZE*sizeof(uint32_t)); // @Merk - ganger her med bredden av skjermen (bytePitch)
    
    SDL_SetRenderTarget(renderer, NULL);
        
    //Clear screen
    SDL_RenderClear(renderer);


    SDL_Rect framebuffer_source_dimensions = {
      .x = 0,
      .y = 0,
      .w = DISPLAY_SIZE,
      .h = DISPLAY_SIZE,
    };
    
    //Render texture to screen
    SDL_RenderCopy(renderer, display, &framebuffer_source_dimensions, NULL); // NULL viser til at skjermen skal fylles med hvanåenn var i source_rect
    
    //Update screen
    SDL_RenderPresent(renderer);

    frames_drawn++;
    usleep(33);
#endif
    break;
    
  default:
    printf("FEIL I KJØRING! Fikk ukjent opcode %i. Forsøker du å kjøre vilkårlig data?\n", instr.op);
    exit(1);
  }
}


int main () {
#ifndef DEBUG
  SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO);
  SDL_Window * window = SDL_CreateWindow("EKAGGATA",
                                         SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                         512, 512, 0);
  
  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED /*|SDL_RENDERER_PRESENTVSYNC*/);
  display  = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, 128, 128);
#endif

  
  // Initialiser CPU og minne
  CPU cpu = {};

  cpu.memory      = malloc(4*1024*1024);             
  cpu.storage     = cpu.memory + STORAGE_OFFSET;     // @MERK - Disse to er bare debug-verdier som kan brukes i main-funksjonen. Kan ikke nås
  cpu.framebuffer = cpu.memory + FRAMEBUFFER_OFFSET; // med assembly (enda?)


  Instr* inst_mem = cpu.memory; // Bare et 'alias' for minnet hvor instruksjoner ligger.
  // Brukes bare i main.



  // Piksel-test
  unsigned int* pixels = (unsigned int*)cpu.framebuffer;
  

  puts("\n\n [ INFO ] \n");
  printf("Instr size: %lu bytes\n", sizeof(Instr));
  puts("");
  printf("mem start: %p\n", inst_mem); 
  puts("\n\n [ STARTER PROGRAM ] \n");

  int line = 0;
  
#define X(op, a, b) inst_mem[line++] = ASM(op, a, b);
#include "build/test.xlasm"
#undef X

  while (INSTR(PC).op) {
    Op this_op = INSTR(PC).op;
    
    #ifdef DEBUG
    print_instr(INSTR(PC));
    #endif
 
    eval(&cpu, INSTR(PC));
    if (!(this_op >= JMP && this_op <= JGE)) {
      // Jumps inkrementerer PC av seg selv :)
      cpu.r[PC]++;
    }

    #ifdef DEBUG
    print_state(&cpu);
    read(0, 0, 1); // @MERK - trykk enter for å steppe i kjøringen.
    #endif
    

  }
#ifdef DEBUG
  print_state(&cpu);
#endif

  printf(" Frames tegnet: %i\n", frames_drawn);
#ifndef DEBUG
  SDL_DestroyTexture(display);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
#endif

  // print_state(&cpu);

  

  
}
