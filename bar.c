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
            if (mode == 1 || mode == 2)
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
