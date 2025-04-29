#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define MEMORY_SIZE 1024

//Global Variables
uint32_t memory[MEMORY_SIZE];  // 4KB memory = 1024 x 4-byte words
int32_t registers[32];         // 32 general-purpose registers
uint32_t PC = 0;               // Program Counter
int total_instructions = 0;    // Total instructions executed
int arithmetic_count = 0;      // Arithmetic instructions count
int logical_count = 0;        // Logical instructions count
int memory_count = 0;         // Memory instructions count
int control_count = 0;        // Control instructions count

// Function Prototypes
const char* get_instruction_name(uint8_t opcode);
int file_read(const char *filename);
void print_contents(int start, int end);
void decode_instruction(uint32_t instruction);
void execute_instruction(uint32_t instruction);

// Function to read memory image from file
int file_read(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("The file could not be opened.\n");
        exit(EXIT_FAILURE);
    }

    char line[20];
    int index = 0;

    while (fgets(line, sizeof(line), file) != NULL && index < MEMORY_SIZE) {
        uint32_t word = (uint32_t)strtoul(line, NULL, 16);
        memory[index++] = word;
    }

    fclose(file);

    if (index == 0) {
        printf("Error: The file is empty. No instructions read.\n");
        exit(EXIT_FAILURE);  // Exit the program immediately
    }

    printf("File Content Loaded. Number of instructions read: %d.\n", index);
    return index;
}


// Function to print memory contents
void print_contents(int start, int end) {
    printf("\n--- Loaded Memory Contents ---\n");
    for (int i = start; i <= end && i < MEMORY_SIZE; i++) {
        printf("0x%04X : 0x%08X : ", i * 4, memory[i]);
        for (int b = 31; b >= 0; b--) {
            printf("%d", (memory[i] >> b) & 1);
            if (b % 4 == 0) printf(" ");
        }
        printf("\n");
    }
}

// Decode and show human-readable instruction
void decode_instruction(uint32_t instruction) {
    uint8_t opcode = (instruction >> 26) & 0x3F;
    uint8_t rs = (instruction >> 21) & 0x1F;
    uint8_t rt = (instruction >> 16) & 0x1F;
    uint8_t rd = (instruction >> 11) & 0x1F;
    int16_t imm = instruction & 0xFFFF;

    if (imm & 0x8000) {
        imm |= 0xFFFF0000;
    }

    const char* instr_name = get_instruction_name(opcode);

    printf("\nDecoded Instruction: 0x%08X\n", instruction);
    printf("  Instruction: %s\n", instr_name);
    printf("  Opcode: 0x%02X\n", opcode);
    printf("  Rs: R%d, Rt: R%d, Rd: R%d, Imm: %d\n", rs, rt, rd, imm);

    printf("  As MIPS-style: ");
    if (opcode <= 0x0A) { // Arithmetic/Logical
        if (opcode % 2 == 0) {
            printf("%s R%d, R%d, R%d\n", instr_name, rd, rs, rt);
        } else {
            printf("%s R%d, R%d, %d\n", instr_name, rt, rs, imm);
        }
    } else if (opcode == 0x0C || opcode == 0x0D) { // LDW / STW
        printf("%s R%d, R%d, %d\n", instr_name, rt, rs, imm);
    } else if (opcode == 0x0E) { // BZ
        printf("%s R%d, %d\n", instr_name, rs, imm);
    } else if (opcode == 0x0F) { // BEQ
        printf("%s R%d, R%d, %d\n", instr_name, rs, rt, imm);
    } else if (opcode == 0x10) { // JR
        printf("%s R%d\n", instr_name, rs);
    } else if (opcode == 0x11) { // HALT
        printf("%s\n", instr_name);
    } else {
        printf("Unknown format\n");
    }
}

// Execute the instruction
void execute_instruction(uint32_t instruction) {
    uint8_t opcode = (instruction >> 26) & 0x3F;
    uint8_t rs = (instruction >> 21) & 0x1F;
    uint8_t rt = (instruction >> 16) & 0x1F;
    uint8_t rd = (instruction >> 11) & 0x1F;
    int16_t imm = instruction & 0xFFFF;
    total_instructions++;

if (opcode <= 0x0A) { // Arithmetic or Logical
    if (opcode <= 0x05) {
        arithmetic_count++;
    } else {
        logical_count++;
    }
} else if (opcode == 0x0C || opcode == 0x0D) { // LDW or STW
    memory_count++;
} else if (opcode == 0x0E || opcode == 0x0F || opcode == 0x10 || opcode == 0x11) { // BZ, BEQ, JR, HALT
    control_count++;
}


    if (imm & 0x8000) {
        imm |= 0xFFFF0000;
    }

    switch (opcode) {
        case 0x00: // ADD
            registers[rd] = registers[rs] + registers[rt];
            PC += 4;
            break;
        case 0x01: // ADDI
            registers[rt] = registers[rs] + imm;
            PC += 4;
            break;
        case 0x02: // SUB
            registers[rd] = registers[rs] - registers[rt];
            PC += 4;
            break;
        case 0x03: // SUBI
            registers[rt] = registers[rs] - imm;
            PC += 4;
            break;
        case 0x04: // MUL
            registers[rd] = registers[rs] * registers[rt];
            PC += 4;
            break;
        case 0x05: // MULI
            registers[rt] = registers[rs] * imm;
            PC += 4;
            break;
        case 0x06: // OR
            registers[rd] = registers[rs] | registers[rt];
            PC += 4;
            break;
        case 0x07: // ORI
            registers[rt] = registers[rs] | imm;
            PC += 4;
            break;
        case 0x08: // AND
            registers[rd] = registers[rs] & registers[rt];
            PC += 4;
            break;
        case 0x09: // ANDI
            registers[rt] = registers[rs] & imm;
            PC += 4;
            break;
        case 0x0A: // XOR
            registers[rd] = registers[rs] ^ registers[rt];
            PC += 4;
            break;
        case 0x0B: // XORI
            registers[rt] = registers[rs] ^ imm;
            PC += 4;
            break;
        case 0x0C: // LDW
            registers[rt] = memory[(registers[rs] + imm) / 4];
            PC += 4;
            break;
        case 0x0D: // STW
            memory[(registers[rs] + imm) / 4] = registers[rt];
            PC += 4;
            break;
        case 0x0E: // BZ
            if (registers[rs] == 0)
                PC += imm * 4;
            else
                PC += 4;
            break;
        case 0x0F: // BEQ
            if (registers[rs] == registers[rt])
                PC += imm * 4;
            else
                PC += 4;
            break;
        case 0x10: // JR
            PC = registers[rs];
            break;
        case 0x11: // HALT
            printf("\nHALT encountered. Stopping simulation.\n");
            printf("\n--- Simulation Summary ---\n");
            printf("Total Instructions Executed: %d\n", total_instructions);
            printf("Arithmetic Instructions: %d\n", arithmetic_count);
            printf("Logical Instructions: %d\n", logical_count);
            printf("Memory Access Instructions: %d\n", memory_count);
            printf("Control Transfer Instructions: %d\n", control_count);
    
    // Print register state
            printf("\nFinal Register States (non-zero only):\n");
            for (int i = 0; i < 32; i++) {
                if (registers[i] != 0) {
                   printf("R%-2d: %d\n", i, registers[i]);
        }
    }
    
    // Optionally, print changed memory contents (if any)
    printf("\nFinal Memory States (non-zero only):\n");
    for (int i = 0; i < MEMORY_SIZE; i++) {
        if (memory[i] != 0) {
            printf("Memory[0x%04X]: 0x%08X\n", i * 4, memory[i]);
        }
    }
            exit(0);
            break;
        default:
            printf("\nUnknown opcode 0x%02X at PC=0x%08X\n", opcode, PC);
            exit(1);
    }
}

// Lookup instruction name based on opcode
const char* get_instruction_name(uint8_t opcode) {
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
        default:   return "UNKNOWN";
    }
}

// Main Function
int main() {
    char filename[100];

    printf("Enter the text filename with the .txt extension please: ");
    if (fgets(filename, sizeof(filename), stdin) != NULL) {
        filename[strcspn(filename, "\n")] = '\0';  // Trim newline

        int words_read = file_read(filename);
        print_contents(0, words_read - 1);

        // Initialize PC and Registers
        PC = 0;
        for (int i = 0; i < 32; i++) registers[i] = 0;

        // Set manual initial values AFTER clearing registers
        registers[4] = 10;
        registers[5] = 20;
        registers[6] = 30;
        registers[7] = 30;
        registers[8] = 1;
        registers[9] = 2;

        // Simulation loop: Run only until last loaded instruction
        while (PC / 4 < words_read) {
            uint32_t instruction = memory[PC / 4];
            decode_instruction(instruction);
            execute_instruction(instruction);
            
        }

        printf("\nSimulation ended normally.\n");
    } else {
        printf("Error reading filename.\n");
    }
    
    
    return 0;
}
