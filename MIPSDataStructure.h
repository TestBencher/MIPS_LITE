#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#define MEMORY_SIZE 4096 // 4KB

// Global Variables
uint32_t memory[MEMORY_SIZE / 4];
bool modified_memory[MEMORY_SIZE / 4] = {false}; // Array to track modified memory
int32_t registers[32];
bool modified_registers[32] = {false}; // Array to track modified registers
uint32_t PC = 0;
bool halt_seen = false;
bool branch_taken = false;
bool branch_delay = false;
uint8_t mode = 0;

int total_instructions = 0;
int arithmetic_count = 0;
int logical_count = 0;
int memory_count = 0;
int control_count = 0;

typedef struct instruction
{
    uint32_t instruction;
} instruction;

typedef struct R_I_type
{
    uint8_t opcode;
    uint8_t rs;
    uint8_t rt;
    uint8_t rd;
    int16_t imm;
    bool R_or_I_type; // true for R-Type and false for I-Type
} R_I_type;

#define PIPELINE_DEPTH 5

typedef struct PipelineStage
{
    instruction raw;
    R_I_type decoded;
    int32_t alu_result;
    int32_t mem_result;
    bool valid;
    bool isStall;
    char *raw_str;
    bool frwd_flags[4]; // 00(0): src1_exe, 01(1): sec2_exe, 10(2): src1_mem, 11(3): src2_mem
} PipelineStage;

PipelineStage pipeline[PIPELINE_DEPTH];

typedef struct HazardPacket
{
    bool isHazard;
    int hazardCnt;
} HazardPacket;

int total_stalls = 0;
int total_cycles = 0;

// typedef struct I_type
// {
//     uint8_t opcode;
//     uint8_t rs;
//     uint8_t rt;
//     int16_t imm;
// } I_type;
