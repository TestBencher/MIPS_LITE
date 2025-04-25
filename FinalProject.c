#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define MEMORY_SIZE 1024
uint32_t memory[MEMORY_SIZE];  // 4KB memory = 1024 x 4-byte words

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
    printf("File Content Loaded. The number of words read: %d These are the words read.\n", index);
    return index;
}

// Function to print memory content in hex and binary
void print_contents(int start, int end) {
    printf("\n Text Contents \n");
    for (int i = start; i <= end && i < MEMORY_SIZE; i++) {
        printf("0x%04X : 0x%08X : ", i * 4, memory[i]);

        for (int b = 31; b >= 0; b--) {
            printf("%d", (memory[i] >> b) & 1);
            if (b % 4 == 0) printf(" ");
        }

        printf("\n");
    }
}

// Function to decode a single 32-bit instruction
void decode_instruction(uint32_t instruction) {
    uint8_t opcode = (instruction >> 26) & 0x3F;       // bits [31:26]
    uint8_t rs = (instruction >> 21) & 0x1F;           // bits [25:21]
    uint8_t rt = (instruction >> 16) & 0x1F;           // bits [20:16]
    uint8_t rd = (instruction >> 11) & 0x1F;           // bits [15:11]
    int16_t imm = instruction & 0xFFFF;                // bits [15:0]

    // Sign-extend the immediate if necessary
    if (imm & 0x8000) {
        imm |= 0xFFFF0000;
    }

    printf("\nDecoded Instruction:\n");
    printf("  The Opcode is: 0x%02X\n", opcode);
    printf("  Rs: R%d\n", rs);
    printf("  Rt: R%d\n", rt);
    printf("  Rd: R%d\n", rd);
    printf("  Imm: %d\n", imm);
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