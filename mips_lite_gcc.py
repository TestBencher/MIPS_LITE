# mips_lite_gcc.py
import sys
import os

OPCODES = {
    'ADD':  0x00, 'ADDI': 0x01,
    'SUB':  0x02, 'SUBI': 0x03,
    'MUL':  0x04, 'MULI': 0x05,
    'OR':   0x06, 'ORI':  0x07,
    'AND':  0x08, 'ANDI': 0x09,
    'XOR':  0x0A, 'XORI': 0x0B,
    'LDW':  0x0C, 'STW':  0x0D,
    'BZ':   0x0E, 'BEQ':  0x0F,
    'JR':   0x10, 'HALT': 0x11,
}

REV_OPCODES = {v: k for k, v in OPCODES.items()}

def encode_instruction(line):
    line = line.strip()
    if not line or line.startswith('#'):
        return None

    comment = ''
    if '#' in line:
        line, comment = line.split('#', 1)
        line = line.strip()
        comment = comment.strip()

    parts = line.split()
    op = parts[0].upper()
    opcode = OPCODES.get(op)
    if opcode is None:
        raise ValueError(f"Unknown opcode: {op}")

    instr = 0
    if op in ('ADD', 'SUB', 'MUL', 'OR', 'AND', 'XOR'):
        rd, rt, rs = [int(p.strip('R,')) for p in parts[1:]]
        instr |= (opcode << 26) | (rs << 21) | (rt << 16) | (rd << 11)

    elif op in ('ADDI', 'SUBI', 'MULI', 'ORI', 'ANDI', 'XORI', 'LDW', 'STW'):
        rt, rs, imm = parts[1:]
        rt = int(rt.strip('R,'))
        rs = int(rs.strip('R,'))
        imm = int(imm) & 0xFFFF
        instr |= (opcode << 26) | (rs << 21) | (rt << 16) | imm

    elif op == 'BZ':
        rs, imm = parts[1:]
        rs = int(rs.strip('R,'))
        imm = int(imm) & 0xFFFF
        instr |= (opcode << 26) | (rs << 21) | imm

    elif op == 'BEQ':
        rs, rt, imm = parts[1:]
        rs = int(rs.strip('R,'))
        rt = int(rt.strip('R,'))
        imm = int(imm) & 0xFFFF
        instr |= (opcode << 26) | (rs << 21) | (rt << 16) | imm

    elif op == 'JR':
        rs = int(parts[1].strip('R,'))
        instr |= (opcode << 26) | (rs << 21)

    elif op == 'HALT':
        instr |= (opcode << 26)

    # return f"# {comment}\n{instr:08X}" if comment else f"{instr:08X}"
    return f"{instr:08X}" if comment else f"{instr:08X}"

def decode_instruction(hexcode):
    instr = int(hexcode, 16)
    opcode = (instr >> 26) & 0x3F
    rs = (instr >> 21) & 0x1F
    rt = (instr >> 16) & 0x1F
    rd = (instr >> 11) & 0x1F
    imm = instr & 0xFFFF
    if imm & 0x8000:
        imm -= 0x10000

    op = REV_OPCODES.get(opcode, 'UNKNOWN')
    if op in ('ADD', 'SUB', 'MUL', 'OR', 'AND', 'XOR'):
        return f"{op} R{rd}, R{rt}, R{rs}"
    elif op in ('ADDI', 'SUBI', 'MULI', 'ORI', 'ANDI', 'XORI', 'LDW', 'STW'):
        return f"{op} R{rt}, R{rs}, {imm}"
    elif op == 'BZ':
        return f"BZ R{rs}, {imm}"
    elif op == 'BEQ':
        return f"BEQ R{rs}, R{rt}, {imm}"
    elif op == 'JR':
        return f"JR R{rs}"
    elif op == 'HALT':
        return "HALT"
    else:
        return f"UNKNOWN 0x{instr:08X}"

def encode_file(input_file):
    output_file = input_file.replace('.s', '.o')
    with open(input_file, 'r') as fin, open(output_file, 'w') as fout:
        for line in fin:
            encoded = encode_instruction(line)
            if encoded:
                fout.write(encoded + '\n')
    print(f"Encoded to {output_file}")

def decode_file(input_file):
    output_file = input_file.replace('.o', '.s')
    with open(input_file, 'r') as fin, open(output_file, 'w') as fout:
        for line in fin:
            if line.startswith('#') or line.strip() == '':
                fout.write(line)
                continue
            decoded = decode_instruction(line.strip())
            fout.write(decoded + '\n')
    print(f"Decoded to {output_file}")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage:")
        print("  python3 mips_lite_gcc.py encode <filename>.s")
        print("  python3 mips_lite_gcc.py decode <filename>.o")
        sys.exit(1)

    mode = sys.argv[1]
    filename = sys.argv[2]

    if mode == 'encode' and filename.endswith('.s'):
        encode_file(filename)
    elif mode == 'decode' and filename.endswith('.o'):
        decode_file(filename)
    else:
        print("Invalid mode or filename.")
