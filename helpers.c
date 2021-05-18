
#include <stdlib.h>
#include <stdio.h>

#include "helpers.h"


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
