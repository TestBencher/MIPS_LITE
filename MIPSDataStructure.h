#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#define MEMORY_SIZE 1024

// Global Variables
uint32_t memory[MEMORY_SIZE];
int32_t registers[32];
bool modified_registers[32] = {false}; // Array to track modified registers
uint32_t PC = 0;

int total_instructions = 0;
int arithmetic_count = 0;
int logical_count = 0;
int memory_count = 0;
int control_count = 0;

typedef struct instruction
{
    uint32_t instruction;
} instruction;

typedef struct R_type
{
    uint8_t opcode;
    uint8_t rs;
    uint8_t rt;
    uint8_t rd;
} R_type;

typedef struct I_type
{
    uint8_t opcode;
    uint8_t rs;
    uint8_t rt;
    int16_t imm;
} I_type;
