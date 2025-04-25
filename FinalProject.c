#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define MEMORY_SIZE 1024
uint32_t memory[MEMORY_SIZE];  // 4KB memory = 1024 x 4-byte words

// Function prototype for instruction name mapping
const char* get_instruction_name(uint8_t opcode);

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
    printf("File Content Loaded. The number of words read: %d.", index);
    return index;
}

// Function to print memory content in hex and binary
void print_contents(int start, int end) {
    printf("\n The Text Contents are: \n");
    for (int i = start; i <= end && i < MEMORY_SIZE; i++) {
        printf("0x%04X : 0x%08X : ", i * 4, memory[i]);

        for (int b = 31; b >= 0; b--) {
            printf("%d", (memory[i] >> b) & 1);
            if (b % 4 == 0) printf(" ");
        }

        printf("\n");
    }
}

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
    printf("  Opcode: 0x%02X\n", opcode);
    printf("  Rs: R%d, Rt: R%d, Rd: R%d, Imm: %d\n", rs, rt, rd, imm);

    // Additional MIPS-style representation
    printf(" MIPS_LITE Instruction : ");
    if (opcode <= 0x0A) { // Arithmetic/Logical
        if (opcode % 2 == 0) {
            // R-type: ADD Rd, Rs, Rt
            printf("%s R%d, R%d, R%d\n", instr_name, rd, rs, rt);
        } else {
            // I-type: ADDI Rt, Rs, Imm
            printf("%s R%d, R%d, %d\n", instr_name, rt, rs, imm);
        }
    } else if (opcode == 0x0C || opcode == 0x0D) {
        // LDW / STW: Rt, Rs, Imm
        printf("%s R%d, R%d, %d\n", instr_name, rt, rs, imm);
    } else if (opcode == 0x0E) {
        // BZ: Rs, offset
        printf("%s R%d, %d\n", instr_name, rs, imm);
    } else if (opcode == 0x0F) {
        // BEQ: Rs, Rt, offset
        printf("%s R%d, R%d, %d\n", instr_name, rs, rt, imm);
    } else if (opcode == 0x10) {
        // JR: Rs
        printf("%s R%d\n", instr_name, rs);
    } else if (opcode == 0x11) {
        // HALT
        printf("%s\n", instr_name);
    } else {
        printf("Unknown format\n");
    }
}



// Instruction name lookup
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

int main() {
    char filename[100];

    printf("Enter the memory image filename: ");
    if (fgets(filename, sizeof(filename), stdin) != NULL) {
        filename[strcspn(filename, "\n")] = '\0';  // Trim newline

        int words_read = file_read(filename);
        print_contents(0, words_read - 1);  // Show only the read memory

        for (int i = 0; i < words_read; i++) {
            decode_instruction(memory[i]);
        }
    } else {
        printf("Error reading filename.\n");
    }

    return 0;
}
