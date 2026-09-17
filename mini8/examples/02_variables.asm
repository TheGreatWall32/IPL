; x = 3; y = 4; z = x + y; print(z)
; Address constants; memory is initialized by STORE.
.equ x, 10
.equ y, 11
.equ z, 12

LDI R0, 3
STORE [x], R0
LDI R0, 4
STORE [y], R0

LOAD R0, [x]
LOAD R1, [y]
ADD R0, R1
STORE [z], R0

LOAD R0, [z]
OUT R0
HALT
