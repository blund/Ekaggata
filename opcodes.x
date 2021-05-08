X(NOP,     NOP, NON) \
X(MOV,     MOV, REG) \
X(MOV_imm, MOV, IMM) \
X(ADD,     ADD, REG) \
X(ADD_imm, ADD, IMM) \
X(SUB,     ADD, REG) \
X(SUB_imm, ADD, IMM) \
X(STR_adr, STR, ADR) \
X(LDR_adr, LDR, ADR) \
X(JMP,     JMP, ADR) \
X(JMP_imm, JMP, IMM) \
X(JEQ,     JMP, IMM) \
X(JNE,     JMP, IMM) \
X(JLT,     JMP, IMM) \
X(JGT,     JMP, IMM) \
X(CMP,     CMP, REG) \
X(CMP_imm, CMP, IMM)
