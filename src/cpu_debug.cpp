#include "cpu_debug.h"
#include <iostream>
#include <fstream>
#include <iomanip>

reg8_e get_r(const reg8 r);
reg16_e get_rp(const reg8 r);
reg16_e get_rp2(const reg8 r);
cond_e get_cc(const reg8 r);
alu_e get_alu(const reg8 r);
rot_e get_rot(const reg8 r);
std::string reg8_e_tostring(const reg8_e r);
std::string reg16_e_tostring(const reg16_e r);
std::string cond_e_tostring(const cond_e cc);
std::string alu_e_tostring(const alu_e alu);
std::string rot_e_tostring(const rot_e rot);
void id_execute_cb(reg8 pc, reg8 instr, std::ostream &out);

#define GET_X(x)    ((x & 0xC0) >> 6)
#define GET_Y(x)    ((x & 0x38) >> 3)
#define GET_Z(x)    (x & 0x07)
#define GET_P(x)    ((x & 0x30) >> 4)
#define GET_Q(x)    ((x & 0x08) >> 3)

reg8_e get_r(const reg8 r)
{
    switch(r)
    {
        case 0x00:  return B;
        case 0x01:  return C;
        case 0x02:  return D;
        case 0x03:  return E;
        case 0x04:  return H;
        case 0x05:  return L;
        case 0x06:  return _HL_;
        case 0x07:
        default:    return A;
    }
}
reg16_e get_rp(const reg8 r)
{
    switch(r)
    {
        case 0x00:  return BC;
        case 0x01:  return DE;
        case 0x02:  return HL;
        case 0x03:
        default:    return SP;
    }
}

reg16_e get_rp2(const reg8 r)
{
    switch(r)
    {
        case 0x00:  return BC;
        case 0x01:  return DE;
        case 0x02:  return HL;
        case 0x03:
        default:    return AF;
    }
}

cond_e get_cc(const reg8 r)
{
    switch(r)
    {
        case 0x00:  return NZ;
        case 0x01:  return Z;
        case 0x02:  return NC;
        case 0x03:  return CC;
        case 0x04:  return PO;
        case 0x05:  return PE;
        case 0x06:  return P;
        case 0x07:
        default:    return M;
    }
}

alu_e get_alu(const reg8 r)
{
    switch(r)
    {
        case 0x00:  return ADD_A;
        case 0x01:  return ADC_A;
        case 0x02:  return SUB;
        case 0x03:  return SBC_A;
        case 0x04:  return AND;
        case 0x05:  return XOR;
        case 0x06:  return OR;
        case 0x07:
        default:    return CP;
    }
}

rot_e get_rot(const reg8 r)
{
    switch(r)
    {
        case 0x00:  return RLC;
        case 0x01:  return RRC;
        case 0x02:  return RL;
        case 0x03:  return RR;
        case 0x04:  return SLA;
        case 0x05:  return SRA;
        case 0x06:  return SLL;
        case 0x07:
        default:    return SRL;
    }
}

std::string cond_e_tostring(const cond_e cc)
{
    switch(cc)
    {
        case NZ:    return "NZ";
        case Z:     return "Z";
        case NC:    return "NC";
        case CC:    return "CC";
        case PO:    return "PO";
        case PE:    return "PE";
        case P:     return "P";
        case M:     return "M";
    }
    return "cond_e_?";
}

std::string alu_e_tostring(const alu_e alu)
{
    switch(alu)
    {
        case ADD_A: return "ADD";
        case ADC_A: return "ADC";
        case SUB:   return "SUB";
        case SBC_A: return "SBC";
        case AND:   return "AND";
        case XOR:   return "XOR";
        case OR:    return "OR";
        case CP:    return "CP";
    }
    return "alu_e_?";
}

std::string rot_e_tostring(const rot_e rot)
{
    switch(rot)
    {
        case RLC:   return "RLC";
        case RRC:   return "RRC";
        case RL:    return "RL";
        case RR:    return "RR";
        case SLA:   return "SLA";
        case SRA:   return "SRA";
        case SLL:   return "SLL";
        case SRL:   return "SRL";
    }
    return "rot_e_?";
}

void cpu_debug_print(reg8 pc, reg8 instr, reg8 data8, reg16_2x8 data16, std::ostream &out)
{
    reg8 x = GET_X(instr);
    reg8 y = GET_Y(instr);
    reg8 z = GET_Z(instr);
    reg8 p = GET_P(instr);
    reg8 q = GET_Q(instr);
    reg16_e rp = AF;
    reg8_e r = A, r2 = A;
    cond_e cc = NZ;
    alu_e alu = ADD_A;

    switch(x)
    {
        case 0x00:
            switch(z)
        {
            case 0x00:
                switch(y)
            {
                case 0x00:
                    out << "NOP";
                    return;
                case 0x01:
                    out << "LD ("
                    << "0x" << std::hex << (int)data16.r16 << "), SP";
                    return;
                case 0x02:
                    out << "STOP";
                    return;
                case 0x03:
                    out << "JR " << "0x" << std::hex << (int)data8;
                    return;
                case 0x04:
                case 0x05:
                case 0x06:
                case 0x07:
                    cc = get_cc(y - 4);
                    out << "JR " << cond_e_tostring(cc)
                    << ", " << "0x" << std::hex << (int)data8;
                    return;
                default:    break;
            }
                break;

            case 0x01:
                switch(q)
            {
                case 0x00:
                    rp = get_rp(p);
                    out << "LD " << reg16_e_tostring(rp)
                    << ", " << "0x" << std::hex << (int)data16.r16;
                    return;
                case 0x01:
                    rp = get_rp(p);
                    out << "ADD HL, " << reg16_e_tostring(rp);
                    return;
                default:    break;
            }
                break;

            case 0x02:
                switch((q << 2) | p)
            {
                case 0x00:                      // LD (BC),A
                    out << "LD (BC), A";
                    return;
                case 0x01:                      // LD (DE),A
                    out << "LD (DE), A";
                    return;
                case 0x02:                      // LDI (HL),A
                    out << "LDI (HL), A";
                    return;
                case 0x03:                      // LDD (HL),A
                    out << "LDD (HL), A";
                    return;
                case 0x04:                      // LD A,(BC)
                    out << "LD A, (BC)";
                    return;
                case 0x05:                      // LD A,(DE)
                    out << "LD A, (DE)";
                    return;
                case 0x06:                      // LDI A,(HL)
                    out << "LDI A, (HL)";
                    return;
                case 0x07:                      // LDD A,(HL)
                    out << "LDD A, (HL)";
                    return;
                default:
                    break;
            }
                break;

            case 0x03:
                rp = get_rp(p);
                if(q == 0x00)
                {
                    out << "INC " << reg16_e_tostring(rp);
                }
                else
                {
                    out << "DEC " << reg16_e_tostring(rp);
                }
                return;

            case 0x04:
                r = get_r(y);
                out << "INC " << reg8_e_tostring(r);
                return;

            case 0x05:
                r = get_r(y);
                out << "DEC " << reg8_e_tostring(r);
                return;

            case 0x06:
                r = get_r(y);
                out << "LD " << reg8_e_tostring(r)
                << ", 0x" << std::hex << (int)data8;
                return;

            case 0x07:
                switch(y)
            {
                case 0x00:
                    out << "RLCA";
                    return;
                case 0x01:
                    out << "RRCA";
                    return;
                case 0x02:
                    out << "RLA";
                    return;
                case 0x03:
                    out << "RRA";
                    return;
                case 0x04:
                    out << "DAA";
                    return;
                case 0x05:
                    out << "CPL";
                    return;
                case 0x06:
                    out << "SCF";
                    return;
                case 0x07:
                    out << "CCF";
                    return;
                default:   break;
            }
                break;

            default:    break;
        }
        case 0x01:
            if((z == 0x06) && (y == 0x06))
            {
                break;
            }
            r = get_r(y);
            r2 = get_r(z);
            out << "LD " << reg8_e_tostring(r) << ", "
            << reg8_e_tostring(r2);
            return;

        case 0x02:
            alu = get_alu(y);
            r = get_r(z);
            out << alu_e_tostring(alu) << " A, "
            << reg8_e_tostring(r);
            return;

        case 0x03:
            switch(z)
        {
            case 0x00:
                cc = get_cc(y);
                if(cc == PO)
                {
                    out << "LDH (" << "0x" << std::hex << (int)data8 << "), A";
                    return;
                }
                if(cc == PE)
                {
                    out << "ADD SP, " << "0x" << std::hex << (int)data8;
                    return;
                }
                if(cc == P)
                {
                    out << "LDH A, (" << "0x" << std::hex << (int)data8 << ")";
                    return;
                }
                if(cc == M)
                {
                    out << "LDHL SP, " << "0x" << std::hex << (int)data8;
                    return;
                }
                out << "RET " << cond_e_tostring(cc);
                return;
            case 0x01:
                if(q == 0x00)
                {
                    rp = get_rp2(p);
                    out << "POP " << reg16_e_tostring(rp);
                    return;
                }
                switch(p)
            {
                case 0x00:
                    out << "RET";
                    return;
                case 0x01:
                    out << "RETI";
                    return;
                case 0x02:
                    out << "JP HL";
                    return;
                case 0x03:
                    out << "LD SP, HL";
                    return;
            }
                break;

            case 0x02:
                cc = get_cc(y);
                if(cc == PO)
                {
                    out << "LD (C), A";
                    return;
                }
                if(cc == PE)
                {
                    out << "LD ("
                    << "0x" << std::hex << (int)data16.r16 << "), A";
                    return;
                }
                if(cc == M)
                {
                    out << "LD A, ("
                    << "0x" << std::hex << (int)data16.r16 << ")";
                    return;
                }
                if(cc == P)
                {
                    break;
                }
                out << "JP " << cond_e_tostring(cc) << ", "
                << "0x" << std::hex << (int)data16.r16;
                return;
            case 0x03:
                switch(y)
            {
                case 0x00:
                    out << "JP " << "0x" << std::hex << (int)data16.r16;
                    return;

                case 0x01:
                    return id_execute_cb(pc, data8, out);
                case 0x02:                      // z80-OUT, removed
                case 0x03:                      // z80-IN,  removed
                case 0x04:                      // z80-EX,  removed
                case 0x05:                      // z80-EX,  removed
                        break;
                case 0x06:
                    out << "DI";
                    return;
                case 0x07:
                    out << "EI";
                    return;
                default:    break;
            }
                break;

            case 0x04:
                cc = get_cc(y);
                out << "CALL " << cond_e_tostring(cc) << ", "
                << "0x" << std::hex << (int)data16.r16;
                return;

            case 0x05:
                if(q == 0x00)
                {
                    rp = get_rp2(p);
                    out << "PUSH " << reg16_e_tostring(rp);
                    return;
                }
                switch(p)
            {
                case 0x00:
                    out << "CALL " << "0x" << std::hex << (int)data16.r16;
                    return;
                case 0x01:                      // z80-DD-prefix, removed
                case 0x02:                      // z80-ED-prefix, removed
                case 0x03:  break;              // z80-FD-prefix, removed
            }
                break;

            case 0x06:
                alu = get_alu(y);
                out << alu_e_tostring(alu) << " A, "
                << "0x" << std::hex << (int)data8;
                return;

            case 0x07:
                out << "RST" << "0x" << std::hex << (int)(8*y);
                return;

            default:    break;
        }
            break;
    }

    std::cout << "Unknown instruction 0x" << "0x" << std::hex << (int)instr;
    return;
}

void id_execute_cb(reg8 pc, reg8 instr, std::ostream &out)
{
    reg8 x = GET_X(instr);
    reg8 y = GET_Y(instr);
    reg8 z = GET_Z(instr);
    rot_e rot;
    reg8_e r = get_r(z);

    switch(x)
    {
        case 0x00:
            rot = get_rot(y);
            out << rot_e_tostring(rot) << " "
            << reg8_e_tostring(r);
            return;
        case 0x01:
            out << "BIT " << "0x" << std::hex << (int)y
            << reg8_e_tostring(r);
            return;
        case 0x02:
            out << "RES " << "0x" << std::hex << (int)y
            << reg8_e_tostring(r);
            return;
        case 0x03:
            out << "SET " << "0x" << std::hex << (int)y
            << reg8_e_tostring(r);
            return;
        default:    break;
    }

    std::cout << "Unknown CB instruction 0x" << "0x" << std::hex << (int)pc;
    return;
}
