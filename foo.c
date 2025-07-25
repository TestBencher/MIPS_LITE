int32_t execute_r_i_type_frwd(R_I_type *r_i_type, int32_t ALU_frwd, int32_t MEM_frwd)
{
    int32_t ALU_result = 0;
    total_instructions++; // Increment total instructions counter

    int32_t src1 = -1;
    int32_t src2 = -1;
    printf("frwd flags: 0: %b, 1: %b, 2: %b, 3: %b\n", pipeline[2].frwd_flags[0], pipeline[2].frwd_flags[1], pipeline[2].frwd_flags[2], pipeline[2].frwd_flags[3]);
    printf("ALU_frwd = %d\n", ALU_frwd);
    printf("MEM_frwd = %d\n", MEM_frwd);
    if (pipeline[2].frwd_flags[0])
    {
        src1 = ALU_frwd;
        pipeline[2].frwd_flags[0] = false;
        printf("[ALU_frwd] src1: %d\n", src1);
    }
    else if (pipeline[2].frwd_flags[2])
    {
        src1 = MEM_frwd;
        pipeline[2].frwd_flags[2] = false;
        printf("[MEM_frwd] src1: %d\n", src1);
    }
    else
    {
        src1 = registers[r_i_type->rs];
        printf("reg[R%d] src1: %d\n", r_i_type->rs, src1);
    }
    if (pipeline[2].frwd_flags[1])
    {
        src2 = ALU_frwd;
        pipeline[2].frwd_flags[1] = false;
        printf("[ALU_frwd] src2: %d\n", src2);
    }
    else if (pipeline[2].frwd_flags[3])
    {
        src2 = MEM_frwd;
        pipeline[2].frwd_flags[3] = false;
        printf("[MEM_frwd] src2: %d\n", src2);
    }
    else
    {
        src2 = registers[r_i_type->rt];
        printf("reg[R%d] src2: %d\n", r_i_type->rt, src2);
        // printf("[reg] src2: %d\n", src2);
    }

    printf("src1 = %d, src2 = %d\n", src1, r_i_type->R_or_I_type ? src2 : r_i_type->imm);

    if (r_i_type->R_or_I_type)
    {
        // True for R-Type instructions
        switch (r_i_type->opcode)
        {
        case 0x00: // ADD
            ALU_result = src1 + src2;
            break;
        case 0x02: // SUB
            ALU_result = src1 - src2;
            break;
        case 0x04: // MUL
            ALU_result = src1 * src2;
            break;
        case 0x06: // OR
            ALU_result = src1 | src2;
            break;
        case 0x08: // AND
            ALU_result = src1 & src2;
            break;
        case 0x0A: // XOR
            ALU_result = src1 ^ src2;
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
            ALU_result = src1 + r_i_type->imm;
            break;
        case 0x03: // SUBI
            ALU_result = src1 - r_i_type->imm;
            break;
        case 0x05: // MULI
            ALU_result = src1 * r_i_type->imm;
            break;
        case 0x07: // ORI
            ALU_result = src1 | r_i_type->imm;
            break;
        case 0x09: // ANDI
            ALU_result = src1 & r_i_type->imm;
            break;
        case 0x0B: // XORI
            ALU_result = src1 ^ r_i_type->imm;
            break;
        case 0x0C: // LDW
        {
            ALU_result = src1 + r_i_type->imm;
            if (ALU_result < 0 || ALU_result / 4 >= MEMORY_SIZE)
            {
                printf("\n[ERROR] Memory access out of bounds at address 0x%08X\n", ALU_result);
                exit(1);
            }
        }
        break;
        case 0x0D: // STW
        {
            ALU_result = src1 + r_i_type->imm;
            if (ALU_result < 0 || ALU_result / 4 >= MEMORY_SIZE)
            {
                printf("\n[ERROR] Memory access out of bounds at address 0x%08X\n", ALU_result);
                exit(1);
            }
        }
        break;
        case 0x0E: // BZ
            if (src1 == 0)
            {
                PC -= 4;
                branch_taken = true;
                if (mode == 1 || mode == 2)
                    PC -= 8;
                PC += r_i_type->imm * 4;
            }
            else
            {
                branch_taken = false;
            }
            break;
        case 0x0F: // BEQ
            if (src1 == src2)
            {
                PC -= 4;
                branch_taken = true;
                if (mode == 1 || mode == 2)
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
            PC = src1; // Assuming PC is in bytes
            branch_taken = true;
            break;
        case 0x11: // HALT
            printf("\n[INFO] HALT instruction encountered. Terminating simulation.\n");
            control_count++;
            // total_cycles++;
            if (mode == 2)
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
