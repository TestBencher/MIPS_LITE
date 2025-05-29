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

char *get_decode_str(instruction raw)
{
    uint32_t instr = raw.instruction;
    uint8_t opcode = (instr >> 26) & 0x3F; // Extract opcode (6 bits)
    uint8_t rs, rt, rd;
    int16_t imm;
    char *decodedInst = malloc(100 * sizeof(char));
    sprintf(decodedInst, "NULL");
    // Define R-type opcodes explicitly
    if (opcode == 0x00 || opcode == 0x02 || opcode == 0x04 || opcode == 0x06 ||
        opcode == 0x08 || opcode == 0x0A) // R-type instruction opcodes
    {
        rs = (instr >> 21) & 0x1F; // Extract Rs (5 bits)
        rt = (instr >> 16) & 0x1F; // Extract Rt (5 bits)
        rd = (instr >> 11) & 0x1F; // Extract Rd (5 bits)

        sprintf(decodedInst, "%s R%d, R%d, R%d", get_instruction_name(opcode), rd, rt, rs);
        // Debug output
        // printf("DEBUG: Decoded R-type Instruction: %s R%d, R%d, R%d\n",
        //        get_instruction_name(opcode), r_i_type->rs, r_i_type->rt, r_i_type->rd);
        // printf("DEBUG: Opcode: %4x, Rs: %4x, Rt: %4x, Rd: %4x\n", opcode, r_i_type->rs, r_i_type->rt, r_i_type->rd);
    }
    else // I-type instruction
    {
        rs = (instr >> 21) & 0x1F; // Extract Rs (5 bits)
        rt = (instr >> 16) & 0x1F; // Extract Rt (5 bits)
        imm = instr & 0xFFFF;      // Extract Imm (16 bits)

        // Sign-extend the immediate value
        if (imm & 0x8000)
            imm |= 0xFFFF0000;

        sprintf(decodedInst, "%s R%d, R%d, %d", get_instruction_name(opcode), rt, rs, imm);
        // Debug output
        // printf("DEBUG: Decoded I-type Instruction: %s R%d, R%d, %d\n",
        //        get_instruction_name(opcode), r_i_type->rs, r_i_type->rt, r_i_type->imm);
        // printf("DEBUG: Opcode: %4x, Rs: %4x, Rt: %4x, Imm: %4x\n", opcode, r_i_type->rs, r_i_type->rt, r_i_type->imm);
    }
    return decodedInst;
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
void decode(instruction fetched_instr, R_I_type *r_i_type)
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
               get_instruction_name(opcode), r_i_type->rd, r_i_type->rt, r_i_type->rs);
        printf("DEBUG: Opcode: %4x, Rd: %4x, Rt: %4x, Rs: %4x\n", opcode, r_i_type->rd, r_i_type->rt, r_i_type->rs);
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

        // check if we have HALT
        if (opcode == 0x11)
        {
            halt_seen = true;
        }

        // Debug output
        printf("DEBUG: Decoded I-type Instruction: %s R%d, R%d, %d\n",
               get_instruction_name(opcode), r_i_type->rt, r_i_type->rs, r_i_type->imm);
        printf("DEBUG: Opcode: %4x, Rt: %4x, Rs: %4x, Imm: %4x\n", opcode, r_i_type->rt, r_i_type->rs, r_i_type->imm);
    }
}

void halt_summary()
{
    printf("\n--- Simulation Summary ---\n");
    printf("- Program Counter (PC): %d\n", PC);
    printf("- Total Clock Cycles: %d\n", total_cycles);
    printf("- Total Stalls: %d\n", total_stalls);
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

    // exit(0);
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
                branch_taken = true;
                if (mode == 1)
                    PC -= 8;
                PC += r_i_type->imm * 4;
            }
            else
            {
                branch_taken = false;
            }
            break;
        case 0x0F: // BEQ
            if (registers[r_i_type->rs] == registers[r_i_type->rt])
            {
                PC -= 4;
                branch_taken = true;
                if (mode == 1)
                    PC -= 8;
                PC += r_i_type->imm * 4;
            }
            else
            {
                branch_taken = false;
            }
            break;
        case 0x10: // JR
            PC -= 4;
            PC = registers[r_i_type->rs]; // Assuming PC is in bytes
            branch_taken = true;
            break;
        case 0x11: // HALT
            printf("\n[INFO] HALT instruction encountered. Terminating simulation.\n");
            control_count++;
            // total_cycles++;
            PC -= 4;
            halt_summary();
            exit(EXIT_FAILURE);
            break;
        default:
            printf("\n[ERROR] [EXE] Unknown I-type opcode: 0x%02X\n", r_i_type->opcode);
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
        default:
            printf("\n[ERROR] [WB] Unknown I-type opcode: 0x%02X\n", r_i_type->opcode);
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

void functional_simulator(int words_read)
{
    int32_t ALU_result, mem_result = 0;
    while (PC / 4 < words_read)
    {
        printf("\nDEBUG: Fetching instruction at PC = 0x%08X\n", PC);
        instruction fetched_instr = fetch();

        printf("DEBUG: Decoding instruction 0x%08X\n", fetched_instr.instruction);
        R_I_type r_i_type = {0};
        decode(fetched_instr, &r_i_type);

        printf("DEBUG: Executing instruction\n");
        ALU_result = execute_r_i_type(&r_i_type);

        printf("DEBUG: MEM Stage\n");
        mem_result = run_mem_stage(ALU_result, &r_i_type);

        printf("DEBUG: Write Back Stage\n");
        run_wb_stage(mem_result, &r_i_type);

        // Print modified registers
        printModRegs();
    }
}

uint8_t shift_pipeline(uint8_t hazardCnt)
{
    // Shift WB, MEM, EX stages normally
    pipeline[4] = pipeline[3];
    pipeline[3] = pipeline[2];

    if (hazardCnt > 0)
    {
        // Insert NOP into EX
        pipeline[2].isStall = true;
        pipeline[1].isStall = true;
        pipeline[0].isStall = true;
        // total_stalls++;
        hazardCnt--;
        // ID and IF stages remain
    }
    else if (branch_taken)
    {
        pipeline[2].isStall = true;
        pipeline[1].isStall = true;
        pipeline[0].isStall = false;
        branch_taken = false;
        branch_delay = true;
        // total_stalls++;
    }
    else
    {
        pipeline[2] = pipeline[1];
        pipeline[1] = pipeline[0];
        if (branch_delay)
        {
            pipeline[1].isStall = false;
            pipeline[2].isStall = true;
            // total_stalls++;
            branch_delay = false;
        }
        else
        {
            pipeline[2].isStall = false;
            pipeline[1].isStall = false;
        }

        // pipeline[2].isStall = false;
        // pipeline[1].isStall = false;
        pipeline[0].isStall = false;
    }
    return hazardCnt;
}

uint8_t has_RAW_hazard(R_I_type *curr, R_I_type *ex, R_I_type *mem)
{
    uint8_t hazardCnt = 0;
    uint8_t src1 = curr->rs;
    uint8_t src2 = (curr->opcode == 0x0F || curr->R_or_I_type) ? curr->rt : 0; // Rt only used in BEQ or R-type

    if (halt_seen)
    {
        printf("DEBUG: HALT instruction encountered, terminating the simulation after draining the pipeline!\n");
        // total_stalls++;
        return 2;
    }
    if (mem && (mem->opcode <= 0x0B || mem->opcode == 0x0C))
    {
        uint8_t mem_dst = mem->R_or_I_type ? mem->rd : mem->rt;
        if ((src1 && src1 == mem_dst) || (src2 && src2 == mem_dst))
        {
            hazardCnt = 1;
            // total_stalls++;
        }
    }
    if (ex && (ex->opcode <= 0x0B || ex->opcode == 0x0C))
    {
        uint8_t ex_dst = ex->R_or_I_type ? ex->rd : ex->rt;
        if ((src1 && src1 == ex_dst) || (src2 && src2 == ex_dst))
        {
            hazardCnt = 2;
            // total_stalls++;
        }
    }

    return hazardCnt;
}

uint8_t has_RAW_hazard_forwarding(R_I_type *curr, R_I_type *ex)
{
    if (!curr)
        return 0;

    uint8_t src1 = curr->rs;
    uint8_t src2 = (curr->opcode == 0x0F || curr->R_or_I_type) ? curr->rt : 0;

    // Only stall if EX stage has LDW and destination matches src1/src2
    if (ex && ex->opcode == 0x0C) // LDW
    {
        uint8_t ex_dst = ex->R_or_I_type ? ex->rd : ex->rt;
        if ((src1 && src1 == ex_dst) || (src2 && src2 == ex_dst))
        {
            return 1; // Stall due to LDW hazard
        }
    }

    return 0; // No hazard requiring stall
}

void print_pipeline()
{
    printf("DEBUG: Pipeline contents -\n");
    if (pipeline[0].valid)
    {
        if (!pipeline[0].isStall)
            printf("IF: %s\n", pipeline[0].raw_str);
        else
            printf("IF: Stall\n");
    }
    if (pipeline[1].valid)
    {
        if (!pipeline[1].isStall)
            printf("ID: %s\n", pipeline[1].raw_str);
        else
            printf("ID: Stall\n");
    }
    if (pipeline[2].valid)
    {
        if (!pipeline[2].isStall)
            printf("EX: %s\n", pipeline[2].raw_str);
        else
            printf("EX: Stall\n");
    }
    if (pipeline[3].valid)
    {
        if (!pipeline[3].isStall)
            printf("MEM: %s\n", pipeline[3].raw_str);
        else
            printf("MEM: Stall\n");
    }
    if (pipeline[4].valid)
    {
        if (!pipeline[4].isStall)
            printf("WB: %s\n", pipeline[4].raw_str);
        else
            printf("WB: Stall\n");
    }
    printf("\n");
}

void print_struct(PipelineStage pipe)
{
    printf("pipeline.raw_str: %s\n", pipe.raw_str);
    printf("pipeline.alu_result: %d\n", pipe.alu_result);
    printf("pipeline.mem_result: %d\n", pipe.mem_result);
    printf("pipeline.valid: %b\n", pipe.valid);
    printf("pipeline.isStall: %b\n", pipe.isStall);
}

void pipeline_simulator_no_forwarding(int words_read)
{
    memset(pipeline, 0, sizeof(pipeline));
    uint8_t hazardCnt = 0;
    int32_t ALU_result, mem_result = 0;
    while (PC / 4 < words_read)
    {
        printf("\nDEBUG: NEW LOOP START\n");

        total_cycles++;
        // if (pipeline[0].valid || pipeline[1].valid || pipeline[2].valid || pipeline[3].valid || pipeline[4].valid)
        // {
        //     total_cycles++;
        // }
        if (!pipeline[0].isStall && !halt_seen)
        {
            printf("\nDEBUG: Fetching instruction at PC = 0x%08X\n", PC);
            pipeline[0].raw = fetch();
            pipeline[0].raw_str = get_decode_str(pipeline[0].raw);
            pipeline[0].valid = true;
        }
        if (pipeline[1].valid && !pipeline[1].isStall && !halt_seen)
        {
            printf("DEBUG: Decoding instruction 0x%08X\n", pipeline[0].raw.instruction);
            decode(pipeline[1].raw, &pipeline[1].decoded);
            // check for hazard
            hazardCnt = has_RAW_hazard(&pipeline[1].decoded, &pipeline[2].decoded, &pipeline[3].decoded);
            // hazardCnt = has_RAW_hazard_forwarding(&pipeline[1].decoded, &pipeline[2].decoded);
            if (!halt_seen)
                total_stalls += hazardCnt;
        }

        if (pipeline[2].valid && !pipeline[2].isStall)
        {
            print_struct(pipeline[2]);
            printf("DEBUG: Executing instruction\n");
            pipeline[2].alu_result = execute_r_i_type(&pipeline[2].decoded);
        }

        if (pipeline[3].valid && !pipeline[3].isStall)
        {
            printf("DEBUG: MEM Stage\n");
            pipeline[3].mem_result = run_mem_stage(pipeline[3].alu_result, &pipeline[3].decoded);
        }

        if (pipeline[4].valid && !pipeline[4].isStall)
        {
            printf("DEBUG: Write Back Stage\n");
            run_wb_stage(pipeline[4].mem_result, &pipeline[4].decoded);
        }
        // Print modified registers
        printf("DEBUG: hazardCnt = %d\n", hazardCnt);
        printModRegs();
        print_pipeline();
        halt_summary();
        hazardCnt = shift_pipeline(hazardCnt);
    }
}

int main(int argc, char *argv[])
{
    if (argc != 3) // Check if the filename is provided as an argument
    {
        printf("Usage: %s <Filename> <Mode>\n", argv[0]);
        printf("<Filename>: input mem filename\n");
        printf("<Mode>: 0/1/2\n");
        printf("\t 0 - Functional Simulator\n");
        printf("\t 1 - Pipeline Simulator with Forwarding\n");
        printf("\t 2 - Pipeline Simulator without Forwarding\n");
        return 1;
    }

    const char *filename = argv[1]; // Get the filename from the command-line argument
    mode = atoi(argv[2]);           // Get the mode to run

    int words_read = file_read(filename);
    // print_contents(0, words_read - 1);

    PC = 0;
    for (int i = 0; i < 32; i++)
    {
        registers[i] = 0;
        modified_registers[i] = false; // Initialize modified registers
    }

    switch (mode)
    {
    case 0:
        // Functional Simulator
        functional_simulator(words_read);
        break;
    case 1:
        // Pipeline Sumulator without Forwarding
        pipeline_simulator_no_forwarding(words_read);
        break;
    case 2:
        // Pipeline Sumulator with Forwarding
        break;
    default:
        printf("\nINVALID MODE enteted!\nPlease enter a valid mode - 0/1/2\n\n");
        printf("Usage: %s <Filename> <Mode>\n", argv[0]);
        printf("<Filename>: input mem filename\n");
        printf("<Mode>: 0/1/2\n");
        printf("\t 0 - Functional Simulator\n");
        printf("\t 1 - Pipeline Simulator with Forwarding\n");
        printf("\t 2 - Pipeline Simulator without Forwarding\n");
        exit(1);
    }

    printf("\n[WARN] Simulation ended without HALT.\n");
    return 0;
}
