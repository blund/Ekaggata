MOV_imm, R7,  16384  // Hvor mange iterasjoner vi vil ha
MOV_imm, R0,  FRAMEBUFFER_OFFSET
MOV_imm, R1,  0xde000000
STR_adr, R1,  R0
ADD_imm, R0,  0x4    // Inkrementer hvilken piksel vi skriver til
ADD_imm, R1,  0x200  // Inkrementer fargen

SUB_imm, R7,  1      // Reduser gjenværende piksler
CMP_imm, R7,  0      // Sjekk om vi har skrevet alle piksler
JEQ,     100, 0      // Om så, hopp til random tom instruksjon
JMP_imm, 3,   0      // Ellers, hopp tilbake til linje 3





