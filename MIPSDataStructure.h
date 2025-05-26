#ifndef MIPS_DATA_STRUCTURE_H
#define MIPS_DATA_STRUCTURE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#define MEMORY_SIZE 4096 // 4KB
#define NUM_REGISTERS 32

// Global Variables
extern uint32_t memory[MEMORY_SIZE / 4];
extern int32_t registers[NUM_REGISTERS];
extern bool modified_registers[NUM_REGISTERS];
extern uint32_t PC;

extern int total_instructions;
extern int arithmetic_count;
extern int logical_count;
extern int memory_count;
extern int control_count;

// Timing Counters
extern int clock_cycles;
extern int stall_count;

// Instruction Structures
typedef struct instruction {
    uint32_t instruction;
} instruction;

typedef struct R_type {
    uint8_t opcode;
    uint8_t rs;
    uint8_t rt;
    uint8_t rd;
} R_type;

typedef struct I_type {
    uint8_t opcode;
    uint8_t rs;
    uint8_t rt;
    int16_t imm;
} I_type;

// Pipeline Stage Structure
typedef struct {
    instruction instr;
    R_type r_type;
    I_type i_type;
    int type;     // 0 = R-type, 1 = I-type, -1 = NOP
    bool valid;   // true = instruction is valid
    bool stall;   // true = bubble/NOP
    int pc;       // PC of the instruction
} PipelineStage;

#endif
