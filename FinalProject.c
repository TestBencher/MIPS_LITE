/**
 * @file FinalProject.c
 * @brief MIPS-LITE Instruction Set Simulator with functional and pipelined modes.
 * 
 * This program simulates execution of MIPS-like instructions
 * using three modes:
 *   1. Functional Simulation (sequential execution)
 *   2. Pipelined Simulation (no forwarding)
 *   3. Pipelined Simulation (with forwarding)
 * 
 * It supports arithmetic, logic, memory, and control instructions.
 * 
 * @author Sai Vishwesh, Amey Kulkarni, Suraj Shetty, Subramanya Dhananjay
 * @date 2025-04-12
 */

#include "MIPSDataStructure.h"
#include <string.h>

bool modified_registers[32] = {false}; // Track modified registers for simulation
uint32_t PC = 0;  // Program Counter

int clock_cycles = 0; // Total clock cycles for the simulation
int stall_count = 0; // Total stalls encountered

PipelineStage pipeline[5]; // Pipeline stages: IF, ID, EX, MEM, WB

const char *inst_name(uint8_t opcode) {   // Returns the instruction name based on the opcode
    switch (opcode) {
    case 0x00: return "ADD"; 
    case 0x01: return "ADDI";
    case 0x02: return "SUB"; 
    case 0x03: return "SUBI";
    case 0x04: return "MUL"; 
    case 0x05: return "MULI";
    case 0x06: return "OR";  
    case 0x07: return "ORI";
    case 0x08: return "AND"; 
    case 0x09: return "ANDI";
    case 0x0A: return "XOR"; 
    case 0x0B: return "XORI";
    case 0x0C: return "LDW"; 
    case 0x0D: return "STW";
    case 0x0E: return "BZ";  
    case 0x0F: return "BEQ";
    case 0x10: return "JR";  
    case 0x11: return "HALT";
    default: return "UNKNOWN";
    }
}

/**
 * @brief Loads a memory image from a text file into memory.
 *
 * Each line in the file should be a 32-bit hex instruction.
 * 
 * @param filename Path to the text file containing instructions.
 * @return Number of instructions successfully read.
 */

int f_read(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("Error: Cannot open file.\n");
        exit(1);
    }
    char line[1024];
    int index = 0;
    while (fgets(line, sizeof(line), file) && index < MEMORY_SIZE / 4) { // Read until EOF or memory limit (4096/4 = 1024 instructions)
        memory[index++] = strtoul(line, NULL, 16);
    }
    fclose(file);

    if (index == 0) {
        printf("Error: File is empty.\n");
        exit(1);
    }
    return index;
}
/**
 * @brief Prints the contents of memory in hexadecimal.
 * 
 * This function prints memory contents between the given indices
 * in a formatted hexadecimal view. It limits the output to
 * the allocated memory range.
 * 
 * @param start The starting index in memory.
 * @param end The ending index in memory.
 */

void print_contents(int start, int end) {
    for (int i = start; i <= end && i < MEMORY_SIZE / 4; i++) {
        printf("0x%04X : 0x%08X\n", i * 4, memory[i]); // To print in hexadecimal format
    }
}

/**
 * @brief Advances the pipeline by shifting stages.
 * 
 * This function shifts the pipeline stages to the next stage,moving instructions through the pipeline.
 * The first stage (IF) is set to invalid after shifting.
 * IF is at index 0, ID at 1, EX at 2, MEM at 3, and WB at 4.
 */

void inst_pipeline() {
    for (int i = 4; i > 0; i--) {  
        pipeline[i] = pipeline[i - 1];
    }
    pipeline[0].valid = false;
}

/**
 * @brief Gets the stage of the producer instruction for a given destination register. Currently Unused. Do we need this for improving functionality?
 * 
 * This function checks the pipeline stages to find where the destination register
 * was last written to, allowing for forwarding in pipelined execution.
 * 
 * @param dest_reg The destination register to check.
 * @return The stage number (2=EX, 3=MEM, 4=WB) or -1 if not found.
 */

int get_producer_stage(int dest_reg) {
    for (int i = 2; i <= 4; i++) {
        if (!pipeline[i].valid) continue;
        int dest = (pipeline[i].type == 0) ? pipeline[i].r_type.rd : pipeline[i].i_type.rt;
        if (dest == dest_reg && dest != 0) return i; // EX=2, MEM=3, WB=4
    }
    return -1;
}

/**
 * @brief Checks for RAW (Read After Write) hazards in the pipeline.
 * 
 * This function checks if the current instruction has a RAW hazard
 * with any of the instructions in the pipeline stages 2 to 4.
 * 
 * @param curr The current instruction stage to check for hazards.
 * @return true if a RAW hazard is detected, false otherwise.
 */

bool raw_hazard(PipelineStage curr) {
    if (!curr.valid) return false;

    for (int i = 2; i <= 4; i++) {  // Check stages 2 to 4 for hazards
        if (!pipeline[i].valid) continue;

        int dest = (pipeline[i].type == 0) ? pipeline[i].r_type.rd : pipeline[i].i_type.rt;
        int src1 = (curr.type == 0) ? curr.r_type.rs : curr.i_type.rs;  // Source registers of the current instruction. Current instruction can be R-type or I-type.
        int src2 = (curr.type == 0) ? curr.r_type.rt : curr.i_type.rt; 

        if ((src1 == dest || src2 == dest) && dest != 0)
            return true;
    }
    return false;
}

/**
 * @brief Decodes a 32-bit instruction word into a PipelineStage structure.
 * 
 * This function extracts the opcode, source, destination registers, and immediate value
 * from the instruction word and populates the PipelineStage structure accordingly.
 * 
 * @param inst_word The 32-bit instruction word to decode.
 * @param pc The program counter value for this instruction.
 * @return A PipelineStage structure containing the decoded instruction.
 */

PipelineStage decode(uint32_t inst_word, int pc) {
    PipelineStage stage = {0};
    stage.valid = true;
    stage.pc = pc;
    stage.instr.instruction = inst_word; // Store the raw instruction word

    uint8_t opcode = (inst_word >> 26) & 0x3F; // Occupies bits 26-31 (6 bits)
    uint8_t rs = (inst_word >> 21) & 0x1F;  // Occupies bits 21-25 (5 bits)
    uint8_t rt = (inst_word >> 16) & 0x1F;  // Occupies bits 16-20 (5 bits)
    uint8_t rd = (inst_word >> 11) & 0x1F;  // Occupies bits 11-15 (5 bits)
    int16_t imm = inst_word & 0xFFFF;  // Occupies bits 0-15 (16 bits)
    if (imm & 0x8000) imm |= 0xFFFF0000; // Sign-extend immediate value if negative

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

/**
 * @brief Prints a summary of the simulation results.
 * 
 * This function outputs the total instruction counts, final register state,
 * total stalls, final memory state (non-zero contents), and timing statistics.
 */

void summary() {                                         
    printf("\nInstruction counts:\n");
    printf("Total number of instructions: %d\n", total_instructions);
    printf("Arithmetic instructions: %d\n", arithmetic_count);
    printf("Logical instructions: %d\n", logical_count);
    printf("Memory access instructions: %d\n", memory_count);
    printf("Control transfer instructions: %d\n", control_count);                                
    printf("\nFinal register state:\n");                                 
    printf("Program counter: %d\n", PC);
    for (int i = 0; i < 32; i++) {
        if (registers[i] != 0) {
            printf("R%d : %d\n", i, registers[i]);
        }
    }         
    printf("\nTotal stalls: %d\n", stall_count);
    printf("\nFinal Memory State (non-zero only):\n");
    for (int i = 0; i < MEMORY_SIZE / 4; i++) {
        if (memory[i] != 0) {
            printf("Address: %d ,Contents: %d\n", i * 4, memory[i]);
        }
    }
    printf("\nTiming Simulator:\n");
    printf("Total number of clock cycles: %d\n", clock_cycles);
    printf("Program Halted\n");
}

/**
 * @brief Executes the instruction in the given pipeline stage.
 * 
 * This function performs the operation specified by the instruction
 * in the provided pipeline stage, updating registers and memory as needed.
 * It also counts the number of executed instructions for statistics.
 * 
 * @param stage The pipeline stage containing the instruction to execute.
 * @return true if HALT instruction is encountered, false otherwise.
 */

bool execute(PipelineStage stage) {             // Simulates the WB stage 
    if (!stage.valid) return false;
    uint8_t opcode = (stage.type == 0) ? stage.r_type.opcode : stage.i_type.opcode;
    switch (opcode) {
    case 0x00: registers[stage.r_type.rd] = registers[stage.r_type.rs] + registers[stage.r_type.rt]; arithmetic_count++; break;  // Case types above 
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
        summary();  // Halt encountered, print summary
        return true; 
    default:
        printf("[ERROR] Unknown opcode 0x%02X\n", opcode); exit(1);
    }

    total_instructions++;
    return false;
}

/**
 * @brief Simulates the pipelined execution of instructions without forwarding.
 * 
 * This function implements a simple pipeline with 5 stages: IF, ID, EX, MEM, WB.
 * It handles stalls due to RAW hazards and executes instructions in the pipeline.
 * 
 * @param word_count The number of words (instructions) to simulate.
 */

void pipeline_no_forwarding(int word_count) {
    memset(pipeline, 0, sizeof(pipeline));       // Clear the pipeline stages, Initialize to 0 and set invalid
    int fetch_index = 0;                        // track the current instruction to fetch
    while (1) {
        clock_cycles++;                        // Each iteration represents a clock cycle
        if (pipeline[4].valid) {               // If WB stage is valid, execute the instruction
            if (execute(pipeline[4])) break;   // If HALT instruction is encountered, break the loop
        }
        if (pipeline[1].valid && raw_hazard(pipeline[1])) { // Check for RAW hazard in ID stage
            stall_count++;
            pipeline[2].valid = false;          // Stall the EX stage by marking it invalid
            inst_pipeline();                    // Shift the pipeline stages
            continue;
        }
        if (fetch_index < word_count) {   // Check if there are more instructions to fetch 
            pipeline[0] = decode(memory[fetch_index], fetch_index * 4); // Decode that instruction
            fetch_index++;
        } else {
            pipeline[0].valid = false;   // If no more instructions, mark IF stage as invalid
        }
        inst_pipeline();   // Shift the pipeline stages to the next stage
    }
}

/**
 * @brief Simulates the functional execution of instructions (No pipelining).
 * 
 * This function executes instructions sequentially, updating registers and memory.
 * It counts the number of executed instructions and handles HALT instruction.
 * 
 * @param word_count The number of words (instructions) to simulate.
 */

void functional_simulation(int word_count) {
    while (PC / 4 < word_count) {
        uint32_t instruction = memory[PC / 4];
        uint8_t opcode = (instruction >> 26) & 0x3F;
        uint8_t rs = (instruction >> 21) & 0x1F;
        uint8_t rt = (instruction >> 16) & 0x1F;
        uint8_t rd = (instruction >> 11) & 0x1F;
        int16_t imm = instruction & 0xFFFF;
        if (imm & 0x8000) imm |= 0xFFFF0000;

        total_instructions++;

        switch (opcode) {
            case 0x00: // ADD
                registers[rd] = registers[rs] + registers[rt];
                modified_registers[rd] = true;
                arithmetic_count++;
                PC += 4;
                break;
            case 0x01: // ADDI
                registers[rt] = registers[rs] + imm;
                modified_registers[rt] = true;
                arithmetic_count++;
                PC += 4;
                break;
            case 0x02: // SUB
                registers[rd] = registers[rs] - registers[rt];
                modified_registers[rd] = true;
                arithmetic_count++;
                PC += 4;
                break;
            case 0x03: // SUBI
                registers[rt] = registers[rs] - imm;
                modified_registers[rt] = true;
                arithmetic_count++;
                PC += 4;
                break;
            case 0x04: // MUL
                registers[rd] = registers[rs] * registers[rt];
                modified_registers[rd] = true;
                arithmetic_count++;
                PC += 4;
                break;
            case 0x05: // MULI
                registers[rt] = registers[rs] * imm;
                modified_registers[rt] = true;
                arithmetic_count++;
                PC += 4;
                break;
            case 0x06: // OR
                registers[rd] = registers[rs] | registers[rt];
                modified_registers[rd] = true;
                logical_count++;
                PC += 4;
                break;
            case 0x07: // ORI
                registers[rt] = registers[rs] | imm;
                modified_registers[rt] = true;
                logical_count++;
                PC += 4;
                break;
            case 0x08: // AND
                registers[rd] = registers[rs] & registers[rt];
                modified_registers[rd] = true;
                logical_count++;
                PC += 4;
                break;
            case 0x09: // ANDI
                registers[rt] = registers[rs] & imm;
                modified_registers[rt] = true;
                logical_count++;
                PC += 4;
                break;
            case 0x0A: // XOR
                registers[rd] = registers[rs] ^ registers[rt];
                modified_registers[rd] = true;
                logical_count++;
                PC += 4;
                break;
            case 0x0B: // XORI
                registers[rt] = registers[rs] ^ imm;
                modified_registers[rt] = true;
                logical_count++;
                PC += 4;
                break;
            case 0x0C: // LDW
                registers[rt] = memory[(registers[rs] + imm) / 4];
                modified_registers[rt] = true;
                memory_count++;
                PC += 4;
                break;
            case 0x0D: // STW
                memory[(registers[rs] + imm) / 4] = registers[rt];
                memory_count++;
                PC += 4;
                break;
            case 0x0E: // BZ
                if (registers[rs] == 0)
                    PC += imm * 4;
                else
                    PC += 4;
                control_count++;
                break;
            case 0x0F: // BEQ
                if (registers[rs] == registers[rt])
                    PC += imm * 4;
                else
                    PC += 4;
                control_count++;
                break;
            case 0x10: // JR
                PC = registers[rs];
                control_count++;
                break;
            case 0x11: // HALT
                printf("\n[INFO] HALT encountered in functional simulation.\n");
                PC += 4;  // Include HALT step in PC reporting
                summary();
                return;
            default:
                printf("\n[ERROR] Unknown opcode 0x%02X at PC=0x%08X\n", opcode, PC);
                exit(1);
        }
    }

    printf("\n[WARN] Functional simulation ended without HALT.\n");
    summary();
}

/**
 * @brief Simulates the pipelined execution of instructions with forwarding.
 * 
 * This function implements a pipelined execution with forwarding to eliminate stalls
 * when data dependencies are detected. It handles instruction fetching, decoding,
 * execution, memory access, and write-back stages.
 * 
 * @param word_count The number of words (instructions) to simulate.
 */

void pipeline_with_forwarding(int word_count) {
    memset(pipeline, 0, sizeof(pipeline));  // Clear the pipeline stages
    int fetch_index = 0;

    while (1) {
        clock_cycles++;
        if (pipeline[4].valid) {     // If WB stage is valid, execute the instruction
            uint8_t opcode = (pipeline[4].type == 0) ? pipeline[4].r_type.opcode : pipeline[4].i_type.opcode; 
            execute(pipeline[4]);

            if (opcode == 0x11) break;  // If HALT instruction, break the loop
        }

        // Forwarding logic: eliminate stalls if data can be forwarded. A little bit skeptical about this part, Might have to fix it. 

        bool stall_needed = false;  // Flag to indicate if a stall is needed
        if (pipeline[1].valid) {
            int src1 = (pipeline[1].type == 0) ? pipeline[1].r_type.rs : pipeline[1].i_type.rs; 
            int src2 = (pipeline[1].type == 0) ? pipeline[1].r_type.rt : pipeline[1].i_type.rt;   // rt might not be used for I-type instructions but using it for consistency and because I am lazy

            for (int i = 2; i <= 4; i++) {    // Check stages 2 to 4 for hazards
                if (!pipeline[i].valid) continue;  // Skip invalid stages

                int dest = (pipeline[i].type == 0) ? pipeline[i].r_type.rd : pipeline[i].i_type.rt;  // Extract the destination register for the instruction in the current stage 
                if ((src1 == dest || src2 == dest) && dest != 0) {  
                    stall_needed = false;  // Potential hazard but Forwarding available 
                    break;
                }
            }
        }

        if (stall_needed) {
            stall_count++;
            pipeline[2].valid = false;  
            inst_pipeline();
            continue;
        }
        if (fetch_index < word_count) {   // If there are more instructions to fetch, same logic used before
            pipeline[0] = decode(memory[fetch_index], PC);  
            fetch_index++;
            PC += 4;
        } else {
            pipeline[0].valid = false;  // If no more instructions, mark IF stage as invalid
        }

        inst_pipeline();
    }

    summary();   // Print the summary after the simulation ends
}

/**
 * @brief Main function to run the MIPS-LITE simulator.
 * 
 * This function prompts the user for a text file containing MIPS instructions,
 * initializes registers, and allows the user to select a simulation mode.
 * It then runs the selected simulation mode and prints the results.
 * 
 * @return Exit status of the program.
 */

int main() {
    char filename[100];
    int mode = -1;

    printf("Enter the text filename: ");
    fgets(filename, sizeof(filename), stdin);
    filename[strcspn(filename, "\n")] = '\0';

    int word_count = f_read(filename);
    
    //print_contents(0, word_count - 1);

    for (int i = 0; i < 32; i++) {
        registers[i] = 0;
        modified_registers[i] = false;
    }
    PC = 0;

    // Ask user for simulation mode
    printf("\nSelect simulation mode:\n");
    printf("0 - Functional Simulation\n");
    printf("1 - Pipelined Simulation (no forwarding)\n");
    printf("2 - Pipelined Simulation (with forwarding)\n");
    printf("Enter mode (0/1/2): ");
    scanf("%d", &mode);
    getchar(); // consume trailing newline

    if (mode == 0) {
        functional_simulation(word_count);
    } else if (mode == 1) {
        pipeline_no_forwarding(word_count);
    } else if (mode == 2) {
        pipeline_with_forwarding(word_count);
    } else {
        printf("Invalid simulation mode selected.\n");
        return 1;
    }

    return 0;
}
