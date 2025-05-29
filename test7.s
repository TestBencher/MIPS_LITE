# Initialize base values
ADDI R1, R0, 100       # R1 = 100
ADDI R2, R0, 200       # R2 = 200
ADDI R3, R0, 300       # R3 = 300
ADDI R4, R0, 0         # R4 = 0

# RAW hazard chain
ADD R5, R1, R2         # R5 = 300
ADD R6, R5, R3         # RAW: depends on R5
ADD R7, R6, R1         # RAW: depends on R6
ADD R8, R7, R2         # RAW: depends on R7

# Load-use hazard
LDW R9, R1, 0          # R9 = Mem[R1 + 0]
ADD R10, R9, R3        # RAW hazard: R9 used immediately

# Store sequence
ADD R11, R3, R2        # R11 = 500
STW R11, R1, 8         # Mem[R1 + 8] = R11

# Control hazard - BZ taken
ADDI R4, R0, 0         # R4 = 0
BZ R4, 2               # Taken → skip next 2
ADD R12, R1, R1        # Should be flushed
ADD R12, R2, R2        # Should be flushed

# Safe continuation
SUB R13, R2, R1        # R13 = 100

# BEQ not taken
ADDI R14, R0, 1
ADDI R15, R0, 2
BEQ R14, R15, 2        # Not taken
ADD R16, R14, R15      # Executed
ADD R17, R16, R1

# ✅ Correct JR target to PC = 112 (28th instruction)
ADDI R20, R0, 112      # Jump to instruction 28 (PC = 112)
JR R20
ADDI R99, R0, 999      # Should be skipped

# PC = 112 → instruction 28
ADDI R21, R0, 123
ADD R22, R21, R1
ADD R23, R21, R1
HALT
ADD R23, R21, R1
