#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "MIPSDataStructure.h"

// Function Prototypes
const char *
get_instruction_name(uint8_t opcode);
int file_read(const char *filename);
void print_contents(int start, int end);
void decode_instruction(uint32_t instruction);
void execute_instruction(uint32_t instruction);

int file_read(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        printf("The file could not be opened.\n");
        exit(EXIT_FAILURE);
    }

    char line[20];
    int index = 0;

    while (fgets(line, sizeof(line), file) != NULL && index < MEMORY_SIZE)
    {
        uint32_t word = (uint32_t)strtoul(line, NULL, 16);
        memory[index++] = word;
    }

    fclose(file);

    if (index == 0)
    {
        printf("Error: The file is empty. No instructions read.\n");
        exit(EXIT_FAILURE);
    }

    printf("File Content Loaded. Number of instructions read: %d.\n", index);
    return index;
}

void print_contents(int start, int end)
{
    printf("\n--- Loaded Memory Contents ---\n");
    for (int i = start; i <= end && i < MEMORY_SIZE; i++)
    {
        printf("0x%04X : 0x%08X : ", i * 4, memory[i]);
        for (int b = 31; b >= 0; b--)
        {
            printf("%d", (memory[i] >> b) & 1);
            if (b % 4 == 0)
                printf(" ");
        }
        printf("\n");
    }
}

void decode_instruction(uint32_t instruction)
{
    uint8_t opcode = (instruction >> 26) & 0x3F;
    uint8_t rs = (instruction >> 21) & 0x1F;
    uint8_t rt = (instruction >> 16) & 0x1F;
    uint8_t rd = (instruction >> 11) & 0x1F;
    int16_t imm = instruction & 0xFFFF;

    if (imm & 0x8000)
        imm |= 0xFFFF0000;

    const char *instr_name = get_instruction_name(opcode);

    printf("\nDecoded Instruction: 0x%08X\n", instruction);
    printf("  Instruction: %s\n", instr_name);
    printf("  Opcode: 0x%02X\n", opcode);
    printf("  Rs: R%d, Rt: R%d, Rd: R%d, Imm: %d\n", rs, rt, rd, imm);
    printf("  As MIPS-style: ");

    if (opcode == 0x00 || opcode == 0x02 || opcode == 0x04 ||
        opcode == 0x06 || opcode == 0x08 || opcode == 0x0A)
    {
        printf("%s R%d, R%d, R%d\n", instr_name, rd, rs, rt);
    }
    else if (opcode == 0x01 || opcode == 0x03 || opcode == 0x05 ||
             opcode == 0x07 || opcode == 0x09 || opcode == 0x0B)
    {
        printf("%s R%d, R%d, %d\n", instr_name, rt, rs, imm);
    }
    else if (opcode == 0x0C || opcode == 0x0D)
    {
        printf("%s R%d, R%d, %d\n", instr_name, rt, rs, imm);
    }
    else if (opcode == 0x0E)
    {
        printf("%s R%d, %d\n", instr_name, rs, imm);
    }
    else if (opcode == 0x0F)
    {
        printf("%s R%d, R%d, %d\n", instr_name, rs, rt, imm);
    }
    else if (opcode == 0x10)
    {
        printf("%s R%d\n", instr_name, rs);
    }
    else if (opcode == 0x11)
    {
        printf("%s\n", instr_name);
    }
    else
    {
        printf("Unknown format\n");
    }
}

void execute_instruction(uint32_t instruction)
{
    if (instruction == 0x08800000)
    {
        printf("\n[TRAP] HALT encountered at PC = 0x%08X. Stopping simulation.\n", PC);
        goto halt_summary;
    }

    uint8_t opcode = (instruction >> 26) & 0x3F;
    uint8_t rs = (instruction >> 21) & 0x1F;
    uint8_t rt = (instruction >> 16) & 0x1F;
    uint8_t rd = (instruction >> 11) & 0x1F;
    int16_t imm = instruction & 0xFFFF;
    if (imm & 0x8000)
        imm |= 0xFFFF0000;

    total_instructions++;
    if (opcode <= 0x0A)
    {
        if (opcode <= 0x05)
            arithmetic_count++;
        else
            logical_count++;
    }
    else if (opcode == 0x0C || opcode == 0x0D)
    {
        memory_count++;
    }
    else if (opcode >= 0x0E && opcode <= 0x11)
    {
        control_count++;
    }

    switch (opcode)
    {
    case 0x00:
        registers[rd] = registers[rs] + registers[rt];
        PC += 4;
        break;
    case 0x01:
        registers[rt] = registers[rs] + imm;
        PC += 4;
        break;
    case 0x02:
        registers[rd] = registers[rs] - registers[rt];
        PC += 4;
        break;
    case 0x03:
        registers[rt] = registers[rs] - imm;
        PC += 4;
        break;
    case 0x04:
        registers[rd] = registers[rs] * registers[rt];
        PC += 4;
        break;
    case 0x05:
        registers[rt] = registers[rs] * imm;
        PC += 4;
        break;
    case 0x06:
        registers[rd] = registers[rs] | registers[rt];
        PC += 4;
        break;
    case 0x07:
        registers[rt] = registers[rs] | imm;
        PC += 4;
        break;
    case 0x08:
        registers[rd] = registers[rs] & registers[rt];
        PC += 4;
        break;
    case 0x09:
        registers[rt] = registers[rs] & imm;
        PC += 4;
        break;
    case 0x0A:
        registers[rd] = registers[rs] ^ registers[rt];
        PC += 4;
        break;
    case 0x0B:
        registers[rt] = registers[rs] ^ imm;
        PC += 4;
        break;
    case 0x0C:
        registers[rt] = memory[(registers[rs] + imm) / 4];
        PC += 4;
        break;
    case 0x0D:
        memory[(registers[rs] + imm) / 4] = registers[rt];
        PC += 4;
        break;
    case 0x0E:
        PC += (registers[rs] == 0) ? imm * 4 : 4;
        break;
    case 0x0F:
        PC += (registers[rs] == registers[rt]) ? imm * 4 : 4;
        break;
    case 0x10:
        PC = registers[rs];
        break;
    case 0x11:
        printf("\n[OPCODE] HALT encountered at PC = 0x%08X. Stopping simulation.\n", PC);
        goto halt_summary;
    default:
        printf("\n[ERROR] Unknown opcode 0x%02X at PC=0x%08X\n", opcode, PC);
        exit(1);
    }
    return;

halt_summary:
    printf("\n--- Simulation Summary ---\n");
    printf("Total Instructions Executed: %d\n", total_instructions);
    printf("Arithmetic Instructions: %d\n", arithmetic_count);
    printf("Logical Instructions: %d\n", logical_count);
    printf("Memory Access Instructions: %d\n", memory_count);
    printf("Control Transfer Instructions: %d\n", control_count);

    printf("\nFinal Register States (non-zero only):\n");
    for (int i = 0; i < 32; i++)
    {
        if (registers[i] != 0)
        {
            printf("R%-2d: %d\n", i, registers[i]);
        }
    }

    printf("\nFinal Memory States (non-zero only):\n");
    for (int i = 0; i < MEMORY_SIZE; i++)
    {
        if (memory[i] != 0)
        {
            printf("Memory[0x%04X]: 0x%08X\n", i * 4, memory[i]);
        }
    }

    exit(0);
}

const char *get_instruction_name(uint8_t opcode)
{
    switch (opcode)
    {
    case 0x00:
        return "ADD";
    case 0x01:
        return "ADDI";
    case 0x02:
        return "SUB";
    case 0x03:
        return "SUBI";
    case 0x04:
        return "MUL";
    case 0x05:
        return "MULI";
    case 0x06:
        return "OR";
    case 0x07:
        return "ORI";
    case 0x08:
        return "AND";
    case 0x09:
        return "ANDI";
    case 0x0A:
        return "XOR";
    case 0x0B:
        return "XORI";
    case 0x0C:
        return "LDW";
    case 0x0D:
        return "STW";
    case 0x0E:
        return "BZ";
    case 0x0F:
        return "BEQ";
    case 0x10:
        return "JR";
    case 0x11:
        return "HALT";
    default:
        return "UNKNOWN";
    }
}

int main()
{
    char filename[100];

    printf("Enter the text filename with the .txt extension please: ");
    if (fgets(filename, sizeof(filename), stdin) != NULL)
    {
        filename[strcspn(filename, "\n")] = '\0';

        int words_read = file_read(filename);
        print_contents(0, words_read - 1);

        PC = 0;
        for (int i = 0; i < 32; i++)
            registers[i] = 0;

        // Register & Memory Initialization for test
        registers[4] = 10;
        registers[5] = 20;
        registers[19] = 123;
        memory[(30 + 4) / 4] = 777;

        while (PC / 4 < words_read)
        {
            printf("DEBUG: Executing instruction at PC = 0x%08X\n", PC);
            uint32_t instruction = memory[PC / 4];
            decode_instruction(instruction);
            execute_instruction(instruction);
        }

        printf("\n[WARN] Simulation ended without HALT.\n");
    }
    else
    {
        printf("Error reading filename.\n");
    }
    return 0;
}
