; total = 0; i = 1
; while i < 6:
;     total = total + i
;     i = i + 1
; print(total)

LDI R0, 0       ; R0 = total
LDI R1, 1       ; R1 = i
LDI R2, 6       ; R2 = upper bound (exclusive)
LDI R3, 1       ; R3 = step

loop:
CMP R1, R2
JGE finish
ADD R0, R1
ADD R1, R3
JMP loop

finish:
OUT R0
HALT
