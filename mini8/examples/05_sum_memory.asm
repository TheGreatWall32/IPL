; Sum 1..5 using memory variables.
.equ i, 0
.equ total, 1

LDI R0, 1
STORE [i], R0
LDI R0, 0
STORE [total], R0

loop:
LOAD R0, [i]
LDI R1, 6
CMP R0, R1
JGE finish

LOAD R1, [total]
ADD R1, R0
STORE [total], R1

LDI R1, 1
ADD R0, R1
STORE [i], R0
JMP loop

finish:
LOAD R0, [total]
OUT R0
HALT
