; x = 7
; if x < 10: y = 1
; else:      y = 2
; print(y)
.equ x, 10
.equ y, 11

LDI R0, 7
STORE [x], R0

LOAD R0, [x]
LDI R1, 10
CMP R0, R1
JGE otherwise

LDI R0, 1
STORE [y], R0
JMP end_if

otherwise:
LDI R0, 2
STORE [y], R0

end_if:
LOAD R0, [y]
OUT R0
HALT
