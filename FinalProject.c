#include "MIPSDataStructure.h"
#include <string.h>

uint32_t memory[MEMORY_SIZE / 4];
int32_t registers[32];
bool modified_registers[32] = {false};
uint32_t PC = 0;

int total_instructions = 0;
int arithmetic_count = 0;
int logical_count = 0;
int memory_count = 0;
int control_count = 0;
int clock_cycles = 0;
int stall_count = 0;

PipelineStage pipeline[5];

const char *get_instruction_name(uint8_t opcode) {
    switch (opcode) {
    case 0x00: return "ADD"; case 0x01: return "ADDI";
    case 0x02: return "SUB"; case 0x03: return "SUBI";
    case 0x04: return "MUL"; case 0x05: return "MULI";
    case 0x06: return "OR";  case 0x07: return "ORI";
    case 0x08: return "AND"; case 0x09: return "ANDI";
    case 0x0A: return "XOR"; case 0x0B: return "XORI";
    case 0x0C: return "LDW"; case 0x0D: return "STW";
    case 0x0E: return "BZ";  case 0x0F: return "BEQ";
    case 0x10: return "JR";  case 0x11: return "HALT";
    default: return "UNKNOWN";
    }
}

int file_read(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("Error: Cannot open file.\n");
        exit(1);
    }

    char line[1024];
    int index = 0;
    while (fgets(line, sizeof(line), file) && index < MEMORY_SIZE / 4) {
        memory[index++] = strtoul(line, NULL, 16);
    }
    fclose(file);

    if (index == 0) {
        printf("Error: File is empty.\n");
        exit(1);
    }
    return index;
}

void print_contents(int start, int end) {
    for (int i = start; i <= end && i < MEMORY_SIZE / 4; i++) {
        printf("0x%04X : 0x%08X\n", i * 4, memory[i]);
    }
}

void advance_pipeline() {
    for (int i = 4; i > 0; i--) {
        pipeline[i] = pipeline[i - 1];
    }
    pipeline[0].valid = false;
}

bool has_raw_hazard(PipelineStage curr) {
    if (!curr.valid) return false;

    for (int i = 2; i <= 4; i++) {
        if (!pipeline[i].valid) continue;

        int dest = (pipeline[i].type == 0) ? pipeline[i].r_type.rd : pipeline[i].i_type.rt;
        int src1 = (curr.type == 0) ? curr.r_type.rs : curr.i_type.rs;
        int src2 = (curr.type == 0) ? curr.r_type.rt : curr.i_type.rt;

        if ((src1 == dest || src2 == dest) && dest != 0)
            return true;
    }
    return false;
}

PipelineStage decode(uint32_t inst_word, int pc) {
    PipelineStage stage = {0};
    stage.valid = true;
    stage.pc = pc;
    stage.instr.instruction = inst_word;

    uint8_t opcode = (inst_word >> 26) & 0x3F;
    uint8_t rs = (inst_word >> 21) & 0x1F;
    uint8_t rt = (inst_word >> 16) & 0x1F;
    uint8_t rd = (inst_word >> 11) & 0x1F;
    int16_t imm = inst_word & 0xFFFF;
    if (imm & 0x8000) imm |= 0xFFFF0000;

    if (opcode == 0x00 || opcode == 0x02 || opcode == 0x04 ||
        opcode == 0x06 || opcode == 0x08 || opcode == 0x0A) {
        stage.type = 0;
        stage.r_type = (R_type){opcode, rs, rt, rd};
    } else {
        stage.type = 1;
        stage.i_type = (I_type){opcode, rs, rt, imm};
    }

    return stage;
}

void print_simulation_summary() {
    // 1. Instruction counts
    printf("\nInstruction counts:\n");
    printf("Total number of instructions: %d\n", total_instructions);
    printf("Arithmetic instructions: %d\n", arithmetic_count);
    printf("Logical instructions: %d\n", logical_count);
    printf("Memory access instructions: %d\n", memory_count);
    printf("Control transfer instructions: %d\n", control_count);

    // 2. Final register state
    printf("\nFinal register state:\n");
    printf("Program counter: %d\n", PC);
    for (int i = 0; i < 32; i++) {
        if (registers[i] != 0) {
            printf("R%d : %d\n", i, registers[i]);
        }
    }

    // 3. Total stalls
    printf("\nTotal stalls: %d\n", stall_count);

    // 4. Final memory state (non-zero only)
    printf("\nFinal Memory State (non-zero only):\n");
    for (int i = 0; i < MEMORY_SIZE / 4; i++) {
        if (memory[i] != 0) {
            printf("Address: %d ,Contents: %d\n", i * 4, memory[i]);
        }
    }

    // 5. Timing stats
    printf("\nTiming Simulator:\n");
    printf("Total number of clock cycles: %d\n", clock_cycles);
    printf("Program Halted\n");
}


bool execute(PipelineStage stage) {
    if (!stage.valid) return false;

    uint8_t opcode = (stage.type == 0) ? stage.r_type.opcode : stage.i_type.opcode;

    switch (opcode) {
    case 0x00: registers[stage.r_type.rd] = registers[stage.r_type.rs] + registers[stage.r_type.rt]; arithmetic_count++; break;
    case 0x01: registers[stage.i_type.rt] = registers[stage.i_type.rs] + stage.i_type.imm; arithmetic_count++; break;
    case 0x02: registers[stage.r_type.rd] = registers[stage.r_type.rs] - registers[stage.r_type.rt]; arithmetic_count++; break;
    case 0x03: registers[stage.i_type.rt] = registers[stage.i_type.rs] - stage.i_type.imm; arithmetic_count++; break;
    case 0x04: registers[stage.r_type.rd] = registers[stage.r_type.rs] * registers[stage.r_type.rt]; arithmetic_count++; break;
    case 0x05: registers[stage.i_type.rt] = registers[stage.i_type.rs] * stage.i_type.imm; arithmetic_count++; break;
    case 0x06: registers[stage.r_type.rd] = registers[stage.r_type.rs] | registers[stage.r_type.rt]; logical_count++; break;
    case 0x07: registers[stage.i_type.rt] = registers[stage.i_type.rs] | stage.i_type.imm; logical_count++; break;
    case 0x08: registers[stage.r_type.rd] = registers[stage.r_type.rs] & registers[stage.r_type.rt]; logical_count++; break;
    case 0x09: registers[stage.i_type.rt] = registers[stage.i_type.rs] & stage.i_type.imm; logical_count++; break;
    case 0x0A: registers[stage.r_type.rd] = registers[stage.r_type.rs] ^ registers[stage.r_type.rt]; logical_count++; break;
    case 0x0B: registers[stage.i_type.rt] = registers[stage.i_type.rs] ^ stage.i_type.imm; logical_count++; break;
    case 0x0C: registers[stage.i_type.rt] = memory[(registers[stage.i_type.rs] + stage.i_type.imm) / 4]; memory_count++; break;
    case 0x0D: memory[(registers[stage.i_type.rs] + stage.i_type.imm) / 4] = registers[stage.i_type.rt]; memory_count++; break;
    case 0x0E: if (registers[stage.i_type.rs] == 0) PC += stage.i_type.imm * 4; control_count++; break;
    case 0x0F: if (registers[stage.i_type.rs] == registers[stage.i_type.rt]) PC += stage.i_type.imm * 4; control_count++; break;
    case 0x10: PC = registers[stage.i_type.rs]; control_count++; break;
    case 0x11:
        print_simulation_summary();
        return true;
    default:
        printf("[ERROR] Unknown opcode 0x%02X\n", opcode); exit(1);
    }

    total_instructions++;
    return false;
}

void simulate_pipeline(int word_count) {
    memset(pipeline, 0, sizeof(pipeline));
    int fetch_index = 0;

    while (1) {
        clock_cycles++;

        if (pipeline[4].valid) {
            if (execute(pipeline[4])) break;
        }

        if (pipeline[1].valid && has_raw_hazard(pipeline[1])) {
            stall_count++;
            pipeline[2].valid = false;
            advance_pipeline();
            continue;
        }

        if (fetch_index < word_count) {
            pipeline[0] = decode(memory[fetch_index], fetch_index * 4);
            fetch_index++;
        } else {
            pipeline[0].valid = false;
        }

        advance_pipeline();
    }
}

int main() {
    char filename[100];
    printf("Enter the text filename: ");
    fgets(filename, sizeof(filename), stdin);
    filename[strcspn(filename, "\n")] = '\0';

    int word_count = file_read(filename);

    // REMOVE or comment out this line if present
    // print_contents(0, word_count - 1);

    for (int i = 0; i < 32; i++) registers[i] = 0;
    PC = 0;

    simulate_pipeline(word_count);

    return 0;
}
