#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "MIPSDataStructure.h"

// Function Prototypes
const char *get_instruction_name(uint8_t opcode);
int file_read(const char *filename);
void print_contents(int start, int end);
instruction fetch();
int decode(instruction fetched_instr, R_type *r_type, I_type *i_type);
void execute_r_type(R_type *r_type);
void execute_i_type(I_type *i_type);
void printModRegs();

int file_read(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        printf("The file could not be opened.\n");
        exit(EXIT_FAILURE);
    }

    char line[1024];
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
    printf("\n");
}

// Fetch Stage: Fetches the instruction from memory
instruction fetch()
{
    instruction fetched_instr;
    fetched_instr.instruction = memory[PC / 4];
    PC += 4; // Increment PC to point to the next instruction
    return fetched_instr;
}

// Decode Stage: Decodes the fetched instruction into R_type or I_type
int decode(instruction fetched_instr, R_type *r_type, I_type *i_type)
{
    uint32_t instr = fetched_instr.instruction;
    uint8_t opcode = (instr >> 26) & 0x3F; // Extract opcode (6 bits)

    // Define R-type opcodes explicitly
    if (opcode == 0x00 || opcode == 0x02 || opcode == 0x04 || opcode == 0x06 ||
        opcode == 0x08 || opcode == 0x0A) // R-type instruction opcodes
    {
        r_type->opcode = opcode;
        r_type->rs = (instr >> 21) & 0x1F; // Extract Rs (5 bits)
        r_type->rt = (instr >> 16) & 0x1F; // Extract Rt (5 bits)
        r_type->rd = (instr >> 11) & 0x1F; // Extract Rd (5 bits)

        // Debug output
        printf("DEBUG: Decoded R-type Instruction: %s R%d, R%d, R%d\n",
               get_instruction_name(opcode), r_type->rd, r_type->rs, r_type->rt);
        printf("DEBUG: Opcode: %4x, Rs: %4x, Rt: %4x, Rd: %4x\n", opcode, r_type->rs, r_type->rt, r_type->rd);

        return 0; // Return 0 for R-type
    }
    else // I-type instruction
    {
        i_type->opcode = opcode;
        i_type->rs = (instr >> 21) & 0x1F; // Extract Rs (5 bits)
        i_type->rt = (instr >> 16) & 0x1F; // Extract Rt (5 bits)
        i_type->imm = instr & 0xFFFF;      // Extract Imm (16 bits)

        // Sign-extend the immediate value
        if (i_type->imm & 0x8000)
            i_type->imm |= 0xFFFF0000;

        // Debug output
        printf("DEBUG: Decoded I-type Instruction: %s R%d, R%d, %d\n",
               get_instruction_name(opcode), i_type->rt, i_type->rs, i_type->imm);
        printf("DEBUG: Opcode: %4x, Rs: %4x, Rt: %4x, Imm: %4x\n", opcode, i_type->rs, i_type->rt, i_type->imm);

        return 1; // Return 1 for I-type
    }
}

void halt_summary()
{
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

// Execute Stage: Executes the decoded R-type instruction
void execute_r_type(R_type *r_type)
{
    total_instructions++; // Increment total instructions counter

    switch (r_type->opcode)
    {
    case 0x00: // ADD
        registers[r_type->rd] = registers[r_type->rs] + registers[r_type->rt];
        modified_registers[r_type->rd] = true;
        arithmetic_count++;
        break;
    case 0x02: // SUB
        registers[r_type->rd] = registers[r_type->rs] - registers[r_type->rt];
        modified_registers[r_type->rd] = true;
        arithmetic_count++;
        break;
    case 0x04: // MUL
        registers[r_type->rd] = registers[r_type->rs] * registers[r_type->rt];
        modified_registers[r_type->rd] = true;
        arithmetic_count++;
        break;
    case 0x06: // OR
        registers[r_type->rd] = registers[r_type->rs] | registers[r_type->rt];
        modified_registers[r_type->rd] = true;
        logical_count++;
        break;
    case 0x08: // AND
        registers[r_type->rd] = registers[r_type->rs] & registers[r_type->rt];
        modified_registers[r_type->rd] = true;
        logical_count++;
        break;
    case 0x0A: // XOR
        registers[r_type->rd] = registers[r_type->rs] ^ registers[r_type->rt];
        modified_registers[r_type->rd] = true;
        logical_count++;
        break;
    default:
        printf("\n[ERROR] Unknown R-type opcode: 0x%02X\n", r_type->opcode);
        exit(1);
    }
}

// Execute Stage: Executes the decoded I-type instruction
void execute_i_type(I_type *i_type)
{
    total_instructions++; // Increment total instructions counter

    switch (i_type->opcode)
    {
    case 0x01: // ADDI
        registers[i_type->rt] = registers[i_type->rs] + i_type->imm;
        modified_registers[i_type->rt] = true;
        arithmetic_count++;
        break;
    case 0x03: // SUBI
        registers[i_type->rt] = registers[i_type->rs] - i_type->imm;
        modified_registers[i_type->rt] = true;
        arithmetic_count++;
        break;
    case 0x05: // MULI
        registers[i_type->rt] = registers[i_type->rs] * i_type->imm;
        modified_registers[i_type->rt] = true;
        arithmetic_count++;
        break;
    case 0x07: // ORI
        registers[i_type->rt] = registers[i_type->rs] | i_type->imm;
        modified_registers[i_type->rt] = true;
        logical_count++;
        break;
    case 0x09: // ANDI
        registers[i_type->rt] = registers[i_type->rs] & i_type->imm;
        modified_registers[i_type->rt] = true;
        logical_count++;
        break;
    case 0x0B: // XORI
        registers[i_type->rt] = registers[i_type->rs] ^ i_type->imm;
        modified_registers[i_type->rt] = true;
        logical_count++;
        break;
    case 0x0C: // LDW
    {
        int32_t effective_address = registers[i_type->rs] + i_type->imm;
        if (effective_address < 0 || effective_address / 4 >= MEMORY_SIZE)
        {
            printf("\n[ERROR] Memory access out of bounds at address 0x%08X\n", effective_address);
            exit(1);
        }
        registers[i_type->rt] = memory[effective_address / 4];
        modified_registers[i_type->rt] = true;
        memory_count++;
    }
    break;
    case 0x0D: // STW
    {
        int32_t effective_address = registers[i_type->rs] + i_type->imm;
        if (effective_address < 0 || effective_address / 4 >= MEMORY_SIZE)
        {
            printf("\n[ERROR] Memory access out of bounds at address 0x%08X\n", effective_address);
            exit(1);
        }
        memory[effective_address / 4] = registers[i_type->rt];
        memory_count++;
    }
    break;
    case 0x0E: // BZ
        if (registers[i_type->rs] == 0)
            PC += i_type->imm;
        control_count++;
        break;
    case 0x0F: // BEQ
        if (registers[i_type->rs] == registers[i_type->rt])
            PC += i_type->imm;
        control_count++;
        break;
    case 0x10: // JR
        PC = registers[i_type->rs];
        control_count++;
        break;
    case 0x11: // HALT
        printf("\n[INFO] HALT instruction encountered. Terminating simulation.\n");
        halt_summary();
    default:
        printf("\n[ERROR] Unknown I-type opcode: 0x%02X\n", i_type->opcode);
        exit(1);
    }
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

void printModRegs()
{
    printf("\nModified Registers:\n");
    for (int i = 0; i < 32; i++)
    {
        if (modified_registers[i])
        {
            printf("R%d = %d\n", i, registers[i]);
            // modified_registers[i] = false; // Reset the modified flag
        }
    }
    printf("\n");
}

int main(int argc, char *argv[])
{
    if (argc != 2) // Check if the filename is provided as an argument
    {
        printf("Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    const char *filename = argv[1]; // Get the filename from the command-line argument

    int words_read = file_read(filename);
    print_contents(0, words_read - 1);

    PC = 0;
    for (int i = 0; i < 32; i++)
    {
        registers[i] = 0;
        modified_registers[i] = false; // Initialize modified registers
    }

    // Register & Memory Initialization for test
    // registers[4] = 10;
    // registers[5] = 20;
    // registers[19] = 123;
    // memory[(30 + 4) / 4] = 777;

    while (PC / 4 < words_read)
    {
        printf("\nDEBUG: Fetching instruction at PC = 0x%08X\n", PC);
        instruction fetched_instr = fetch();

        printf("DEBUG: Decoding instruction 0x%08X\n", fetched_instr.instruction);
        R_type r_type = {0};
        I_type i_type = {0};
        int instruction_type = decode(fetched_instr, &r_type, &i_type);

        printf("DEBUG: Executing instruction\n");
        if (instruction_type == 0) // R-type
        {
            execute_r_type(&r_type);
        }
        else if (instruction_type == 1) // I-type
        {
            execute_i_type(&i_type);
        }

        // Print modified registers
        printModRegs();
    }

    printf("\n[WARN] Simulation ended without HALT.\n");
    return 0;
}
