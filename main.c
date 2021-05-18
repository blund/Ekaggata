#include <malloc.h>
#include <limits.h>
#include <unistd.h>


#include "asm.h"
#include "renderer.h"
#include "helpers.h"

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
#define WINDOW_SIZE  512  

#define INSTR(PC) inst_mem[cpu.r[PC]]
#define ASM(opcode, a, b) (Instr){.op = opcode, .reg_to = a, .reg_from = b}

Render_Context context      = {}; // kontekst for tegning
int            frames_drawn = 0;

// https://en.wikipedia.org/wiki/X_Macro

void eval (CPU* cpu, Instr instr) {
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
    render(&context, cpu);
    frames_drawn++;
    break;
    
  default:
    printf("FEIL I KJØRING! Fikk ukjent opcode %i. Forsøker du å kjøre vilkårlig data?\n", instr.op);
    exit(1);
  }
}


int main () {
  setup_graphics(&context, WINDOW_SIZE, WINDOW_SIZE, DISPLAY_SIZE, DISPLAY_SIZE);
  
  // Initialiser CPU og minne
  CPU cpu = {};

  cpu.memory      = malloc(4*1024*1024);             
  cpu.storage     = cpu.memory + STORAGE_OFFSET;     // @MERK - Disse to er bare debug-verdier som kan brukes i main-funksjonen. Kan ikke nås
  cpu.framebuffer = cpu.memory + FRAMEBUFFER_OFFSET; // med assembly (enda?)


  Instr *inst_mem = cpu.memory; // Bare et 'alias' for minnet hvor instruksjoner ligger.
  u32   *pixels   = (u32*)cpu.framebuffer; // Sier hvor renderer skal hente sine piksler :)
  
#ifdef DEBUG
  puts("\n\n [ INFO ] \n");
  printf("Instr size: %lu bytes\n\n", sizeof(Instr));
  printf("mem start: %p\n", inst_mem); 
  puts("\n\n [ STARTER PROGRAM ] \n");
#endif

  
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

  renderer_cleanup(&context);
  
}
