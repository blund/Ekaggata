#ifndef ASM_H
#define ASM_H

#include "base.h"

#define N (1 << 0) // Negativ
#define Z (1 << 1) // Null
#define C (1 << 2) // Carry
#define V (1 << 3) // Overflow


typedef enum Reg {
#define X(name) name,
#include "registers.x"
#undef X
} Reg;

typedef enum Op {
#define X(opcode, index, name, type) opcode,
#include "opcodes.x"
#undef X
} Op;

typedef enum Opcode_Type {
  NON,
  REG,
  IMM,
  ADR,
} Opcode_Type;

typedef struct Instr {
  Op op;

  Reg reg_to;
  
  union {
    Reg reg_from; // Avhengig av suffiks i op_navnet, velges en av disse.
    s32 imm;      // ved lesing. Alle er praktisk talt ints foreløpig, men
    s32 adr;      // kan brukes for floats og andre ting.
  };
  
} Instr;


typedef struct CPU {
  s32 r[32]; // @MERK! - 16 registre allokert, men vi bruker kun 17 (16 + PC)

  Instr *instr;
  
  u8 flags;
  
  void *memory;
  void *storage;
  void *framebuffer;
  
} CPU;

#endif
