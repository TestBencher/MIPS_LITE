#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "MIPSDataStructure.h"

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
int decode(instruction fetched_instr, R_I_type *r_i_type)
{
    uint32_t instr = fetched_instr.instruction;
    uint8_t opcode = (instr >> 26) & 0x3F; // Extract opcode (6 bits)

    // Define R-type opcodes explicitly
    if (opcode == 0x00 || opcode == 0x02 || opcode == 0x04 || opcode == 0x06 ||
        opcode == 0x08 || opcode == 0x0A) // R-type instruction opcodes
    {
        r_i_type->opcode = opcode;
        r_i_type->rs = (instr >> 21) & 0x1F; // Extract Rs (5 bits)
        r_i_type->rt = (instr >> 16) & 0x1F; // Extract Rt (5 bits)
        r_i_type->rd = (instr >> 11) & 0x1F; // Extract Rd (5 bits)
        r_i_type->R_or_I_type = true;        // true for R-Type

        // Debug output
        printf("DEBUG: Decoded R-type Instruction: %s R%d, R%d, R%d\n",
               get_instruction_name(opcode), r_i_type->rs, r_i_type->rt, r_i_type->rd);
        printf("DEBUG: Opcode: %4x, Rs: %4x, Rt: %4x, Rd: %4x\n", opcode, r_i_type->rs, r_i_type->rt, r_i_type->rd);

        return 0; // Return 0 for R-type
    }
    else // I-type instruction
    {
        r_i_type->opcode = opcode;
        r_i_type->rs = (instr >> 21) & 0x1F; // Extract Rs (5 bits)
        r_i_type->rt = (instr >> 16) & 0x1F; // Extract Rt (5 bits)
        r_i_type->imm = instr & 0xFFFF;      // Extract Imm (16 bits)
        r_i_type->R_or_I_type = false;       // false for I-Type

        // Sign-extend the immediate value
        if (r_i_type->imm & 0x8000)
            r_i_type->imm |= 0xFFFF0000;

        // Debug output
        printf("DEBUG: Decoded I-type Instruction: %s R%d, R%d, %d\n",
               get_instruction_name(opcode), r_i_type->rs, r_i_type->rt, r_i_type->imm);
        printf("DEBUG: Opcode: %4x, Rs: %4x, Rt: %4x, Imm: %4x\n", opcode, r_i_type->rs, r_i_type->rt, r_i_type->imm);

        return 1; // Return 1 for I-type
    }
}

void halt_summary()
{
    printf("\n--- Simulation Summary ---\n");
    printf("- Program Counter (PC): %d\n", PC);
    printf("- Total Stalls: %d\n", 0);
    printf("- Total Instructions Executed: %d\n", total_instructions);
    printf("  |- Arithmetic Instructions: %d\n", arithmetic_count);
    printf("  |- Logical Instructions: %d\n", logical_count);
    printf("  |- Memory Access Instructions: %d\n", memory_count);
    printf("  |- Control Transfer Instructions: %d\n", control_count);

    printf("\nFinal Register States (Modified only):\n");
    for (int i = 0; i < 32; i += 4)
    {
        for (int j = i; j < i + 4; j++)
        {
            if (modified_registers[j])
            {
                printf("R%-2d: %5d\t", j, registers[j]);
            }
        }
        if (modified_registers[i] || modified_registers[i + 1] || modified_registers[i + 2] || modified_registers[i + 3])
            printf("\n");
    }

    printf("\nFinal Memory States (Modified only):\n");
    for (int i = 0; i < MEMORY_SIZE / 4; i++)
    {
        if (modified_memory[i])
        {
            printf("Memory[%d]: %d\n", i * 4, memory[i]);
        }
    }

    exit(0);
}

// Execute Stage: Executes the decoded R or I-type instruction
int32_t execute_r_i_type(R_I_type *r_i_type)
{
    int32_t ALU_result = 0;
    total_instructions++; // Increment total instructions counter

    if (r_i_type->R_or_I_type)
    {
        // True for R-Type instructions
        switch (r_i_type->opcode)
        {
        case 0x00: // ADD
            ALU_result = registers[r_i_type->rs] + registers[r_i_type->rt];
            break;
        case 0x02: // SUB
            ALU_result = registers[r_i_type->rs] - registers[r_i_type->rt];
            break;
        case 0x04: // MUL
            ALU_result = registers[r_i_type->rs] * registers[r_i_type->rt];
            break;
        case 0x06: // OR
            ALU_result = registers[r_i_type->rs] | registers[r_i_type->rt];
            break;
        case 0x08: // AND
            ALU_result = registers[r_i_type->rs] & registers[r_i_type->rt];
            break;
        case 0x0A: // XOR
            ALU_result = registers[r_i_type->rs] ^ registers[r_i_type->rt];
            break;
        default:
            printf("\n[ERROR] Unknown R-type opcode: 0x%02X\n", r_i_type->opcode);
            exit(1);
        }
    }
    else
    {
        // False for I-Type instructions
        switch (r_i_type->opcode)
        {
        case 0x01: // ADDI
            ALU_result = registers[r_i_type->rs] + r_i_type->imm;
            break;
        case 0x03: // SUBI
            ALU_result = registers[r_i_type->rs] - r_i_type->imm;
            break;
        case 0x05: // MULI
            ALU_result = registers[r_i_type->rs] * r_i_type->imm;
            break;
        case 0x07: // ORI
            ALU_result = registers[r_i_type->rs] | r_i_type->imm;
            break;
        case 0x09: // ANDI
            ALU_result = registers[r_i_type->rs] & r_i_type->imm;
            break;
        case 0x0B: // XORI
            ALU_result = registers[r_i_type->rs] ^ r_i_type->imm;
            break;
        case 0x0C: // LDW
        {
            ALU_result = registers[r_i_type->rs] + r_i_type->imm;
            if (ALU_result < 0 || ALU_result / 4 >= MEMORY_SIZE)
            {
                printf("\n[ERROR] Memory access out of bounds at address 0x%08X\n", ALU_result);
                exit(1);
            }
        }
        break;
        case 0x0D: // STW
        {
            ALU_result = registers[r_i_type->rs] + r_i_type->imm;
            if (ALU_result < 0 || ALU_result / 4 >= MEMORY_SIZE)
            {
                printf("\n[ERROR] Memory access out of bounds at address 0x%08X\n", ALU_result);
                exit(1);
            }
        }
        break;
        case 0x0E: // BZ
            if (registers[r_i_type->rs] == 0)
            {
                PC -= 4;
                PC += r_i_type->imm * 4;
            }
            break;
        case 0x0F: // BEQ
            if (registers[r_i_type->rs] == registers[r_i_type->rt])
            {
                PC -= 4;
                PC += r_i_type->imm * 4;
            }
            break;
        case 0x10: // JR
            PC -= 4;
            PC = registers[r_i_type->rs]; // Assuming PC is in bytes
            break;
        case 0x11: // HALT
            printf("\n[INFO] HALT instruction encountered. Terminating simulation.\n");
            control_count++;
            halt_summary();
        default:
            printf("\n[ERROR] Unknown I-type opcode: 0x%02X\n", r_i_type->opcode);
            exit(1);
        }
    }
    return ALU_result;
}

int32_t run_mem_stage(int32_t ALU_result, R_I_type *r_i_type)
{
    int32_t fetched_mem = 0;
    switch (r_i_type->opcode)
    {
    case 0x0C: // LDW
    {
        fetched_mem = memory[ALU_result / 4];
    }
    break;
    case 0x0D: // STW
    {
        memory[ALU_result / 4] = registers[r_i_type->rt];
        modified_memory[ALU_result / 4] = true; // Mark memory as modified
    }
    break;
    default:
        fetched_mem = ALU_result;
        break;
    }
}

void run_wb_stage(int32_t fetched_mem, R_I_type *r_i_type)
{

    if (r_i_type->R_or_I_type)
    {
        // True for R-Type instructions
        switch (r_i_type->opcode)
        {
        case 0x00: // ADD
        case 0x02: // SUB
        case 0x04: // MUL
            registers[r_i_type->rd] = fetched_mem;
            modified_registers[r_i_type->rd] = true;
            arithmetic_count++;
            break;
        case 0x06: // OR
        case 0x08: // AND
        case 0x0A: // XOR
            registers[r_i_type->rd] = fetched_mem;
            modified_registers[r_i_type->rd] = true;
            logical_count++;
            break;
        default:
            printf("\n[ERROR] Unknown R-type opcode: 0x%02X\n", r_i_type->opcode);
            exit(1);
        }
    }
    else
    {
        // False for I-Type instructions
        switch (r_i_type->opcode)
        {
        case 0x01: // ADDI
        case 0x03: // SUBI
        case 0x05: // MULI
            registers[r_i_type->rt] = fetched_mem;
            modified_registers[r_i_type->rt] = true;
            arithmetic_count++;
            break;
        case 0x07: // ORI
        case 0x09: // ANDI
        case 0x0B: // XORI
            registers[r_i_type->rt] = fetched_mem;
            modified_registers[r_i_type->rt] = true;
            logical_count++;
            break;
        case 0x0C: // LDW
            registers[r_i_type->rt] = fetched_mem;
            modified_registers[r_i_type->rt] = true;
            memory_count++;
            break;
        case 0x0D: // STW
            memory_count++;
            break;
        case 0x0E: // BZ
        case 0x0F: // BEQ
        case 0x10: // JR
            control_count++;
            break;
        case 0x11: // HALT
            printf("\n[INFO] HALT instruction encountered. Terminating simulation.\n");
        default:
            printf("\n[ERROR] Unknown I-type opcode: 0x%02X\n", r_i_type->opcode);
            exit(1);
        }
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
    int32_t ALU_result, mem_result = 0;
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

    while (PC / 4 < words_read)
    {
        printf("\nDEBUG: Fetching instruction at PC = 0x%08X\n", PC);
        instruction fetched_instr = fetch();

        printf("DEBUG: Decoding instruction 0x%08X\n", fetched_instr.instruction);
        R_I_type r_i_type = {0};
        int instruction_type = decode(fetched_instr, &r_i_type);

        printf("DEBUG: Executing instruction\n");
        ALU_result = execute_r_i_type(&r_i_type);

        printf("DEBUG: MEM Stage\n");
        mem_result = run_mem_stage(ALU_result, &r_i_type);

        printf("DEBUG: Write Back Stage\n");
        run_wb_stage(mem_result, &r_i_type);

        // Print modified registers
        printModRegs();
    }

    printf("\n[WARN] Simulation ended without HALT.\n");
    return 0;
}
