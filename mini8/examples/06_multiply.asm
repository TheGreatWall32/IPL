; 3 * 4 = 12 (repeated addition, modulo 256).
LDI R0, 0       ; result
LDI R1, 3       ; number of repetitions
LDI R2, 4       ; addend

loop:
LDI R3, 0
CMP R1, R3
JE finish
ADD R0, R2
LDI R3, 1
SUB R1, R3
JMP loop

finish:
OUT R0
HALT
