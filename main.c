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

#define INSTR(PC) inst_mem[cpu.r[PC*sizeof(Instr)]]
#define ASM(opcode, a, b) (Instr){.op = opcode, .reg_to = a, .reg_from = b}

Render_Context context      = {}; // kontekst for tegning
int            frames_drawn = 0;

// https://en.wikipedia.org/wiki/X_Macro

// clang-format off

#define REG_TO   cpu->r[cpu->instr->reg_to]
#define REG_FROM cpu->r[cpu->instr->reg_from]
#define IMM      cpu->instr->reg_from

void op_nop     (CPU* cpu) { return; }
void op_mov     (CPU* cpu) { REG_TO =  REG_FROM; }
void op_mov_imm (CPU* cpu) { REG_TO =  IMM; }
void op_add     (CPU* cpu) { REG_TO += REG_FROM; }
void op_add_imm (CPU* cpu) { REG_TO += IMM; }
void op_sub     (CPU* cpu) { REG_TO -= REG_FROM; }
void op_sub_imm (CPU* cpu) { REG_TO -= IMM; }
void op_mul     (CPU* cpu) { REG_TO *= REG_FROM; }
void op_mul_imm (CPU *cpu) { REG_TO *= IMM; }
void op_div     (CPU *cpu) { REG_TO /= REG_FROM; }
void op_div_imm (CPU *cpu) { REG_TO /= IMM; }
void op_ldr     (CPU *cpu) { REG_TO = *(s32 *)(cpu->memory + REG_FROM); }
void op_str     (CPU *cpu) { *(s32 *)(cpu->memory + REG_FROM) = REG_TO; } // @MERK at vi bruker reg_to (første argument) som kilde.
void op_jmp     (CPU *cpu) { cpu->r[PC] = cpu->instr->reg_to - 1; }       // @MERK - vi trekker fra 1 på alle jmp siden vi inkrementerer pc etter instruksjon utført..
void op_jeq     (CPU *cpu) { if (cpu->flags & Z)                                                  cpu->r[PC] = cpu->instr->reg_to - 1; }
void op_jne     (CPU *cpu) { if (!(cpu->flags & Z))                                               cpu->r[PC] = cpu->instr->reg_to - 1; }
void op_jgt     (CPU *cpu) { if (!(cpu->flags & Z) && (!!(cpu->flags & N) == !!(cpu->flags & V))) cpu->r[PC] = cpu->instr->reg_to - 1; }
void op_jge     (CPU *cpu) { if (!!(cpu->flags & N) == !!(cpu->flags & V))                        cpu->r[PC] = cpu->instr->reg_to - 1; }
void op_jlt     (CPU *cpu) { if ((cpu->flags & N) != (cpu->flags & V))                            cpu->r[PC] = cpu->instr->reg_to - 1; } 
void op_jle     (CPU *cpu) { if ((cpu->flags & Z) || (!!(cpu->flags & N) != !!(cpu->flags & V)))  cpu->r[PC] = cpu->instr->reg_to - 1; } 
void op_drw     (CPU *cpu) { render(&context, cpu); frames_drawn++; }
void op_exit    (CPU *cpu) { exit(0); }; // @TODO vet ikke om dette er en fornuftig måte å slutte programmet på...
// clang-format on

void op_cmp (CPU *cpu) {
  // Om hvordan man lager LT, GT osv... fra NZCV
  // https://community.arm.com/developer/ip-products/processors/b/processors-ip-blog/posts/condition-codes-1-condition-flags-and-codes
  
  s32 diff              =  REG_TO - REG_FROM;
  u32 unsigned_overflow = (REG_TO + REG_FROM) < REG_TO;
  u32 signed_overflow   = (REG_TO < 0) && (REG_FROM > (INT_MAX - REG_FROM));

  cpu->flags = 0;
      
  cpu->flags |= diff               ? 0 : Z;
  cpu->flags |= diff < 0           ? N : 0;
  cpu->flags |= unsigned_overflow  ? C : 0; 
  cpu->flags |= signed_overflow    ? V : 0;

}

void op_cmp_imm(CPU *cpu) {
  // Om hvordan man lager LT, GT osv... fra NZCV
  // https://community.arm.com/developer/ip-products/processors/b/processors-ip-blog/posts/condition-codes-1-condition-flags-and-codes
      
  s32 diff              =  REG_TO - cpu->instr->imm;
  u32 unsigned_overflow = (REG_TO + cpu->instr->imm) < REG_TO;
  s32 signed_overflow   = (REG_TO < 0) && (cpu->instr->imm > INT_MAX - cpu->instr->imm);

  cpu->flags = 0;
      
  cpu->flags |= diff               ? 0 : Z;
  cpu->flags |= diff < 0           ? N : 0;
  cpu->flags |= unsigned_overflow  ? C : 0; 
  cpu->flags |= signed_overflow    ? V : 0;
}     


void (*ops[])(CPU *cpu) = { // @MERK henter inn navnene på funksjonene over fra parallelle tabell i opcodes.x, for å sørge for at indekser matcher og at alle er med.
#define X(opcode, index, name, type) index,
#include "opcodes.x"
#undef X
};




int main () {
  setup_graphics(&context, WINDOW_SIZE, WINDOW_SIZE, DISPLAY_SIZE, DISPLAY_SIZE);
  
  // Initialiser CPU og minne
  CPU cpu = {};

  cpu.memory      = malloc(1024*1024);             
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
  int run = 1;
  while (run) {
    cpu.instr = &inst_mem[cpu.r[PC]];


    if (!cpu.instr->op) {
      break;
    }
    
    (*ops[cpu.instr->op])(&cpu);
    
#ifdef DEBUG
    print_instr(*cpu.instr);
#endif

    cpu.r[PC]++;
    

    
#ifdef DEBUG
  printf("flags: %x\n", cpu.flags);
  printf("eq: %i\n", (cpu.flags & Z));
  printf("ge: %i\n", !!(cpu.flags & N) == !!(cpu.flags & V));
  printf("gt: %i\n", !(cpu.flags & Z) && (cpu.flags & N) == (cpu.flags & V));
  printf("lt: %i\n", (cpu.flags & N) != (cpu.flags & V));
#endif

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
