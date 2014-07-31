#include "cpu.h"

#define DEBUG_OUTPUT 0

#define FLAG_Z  0x80
#define FLAG_N  0x40
#define FLAG_H  0x20
#define FLAG_C  0x10

#define FLAG_I_JOYPAD   0x10
#define FLAG_I_SERIAL   0x08
#define FLAG_I_TIMER    0x04
#define FLAG_I_LCDSTAT  0x02
#define FLAG_I_VBLANK   0x01

std::string binstring(const unsigned char byte);
std::string binstring(const unsigned short bytes);
std::string reg8_e_tostring(const reg8_e r);
std::string reg16_e_tostring(const reg16_e r);
std::string cond_e_tostring(const cond_e cc);
std::string alu_e_tostring(const alu_e alu);
std::string rot_e_tostring(const rot_e rot);

void cpu_t::init(membus_t *membus_, bool bootrom_enabled)
{
    last_instr = 0x00;
    last_adr = 0x00;
    panicked = false;
    booted = false;
    halted = false;
    IME = false;
    membus = membus_;
    cycles_left = 0;
    for(int i = 0; i < 6; ++i)  // To make valgrind happy, shouldn't be here.
        registers[i].r16 = 0x00;
    *get_reg(PC) = (bootrom_enabled ? 0x0000 : 0x0100);
    IE = membus->get_pointer(0xFFFF);
    IF = membus->get_pointer(0xFFFE);
}

void cpu_t::run()
{
    if(!panicked)
    {
        if(!halted && cycles_left == 0)
        {
            id_execute();
        }
        if(cycles_left > 0) --cycles_left;
//      inc_counters();
        check_interrupts();
    }
}

int8_t cpu_t::read_mem()
{
    //last_adr = *get_reg(PC);
    int8_t c = membus->read((*get_reg(PC))++);
    return c;
}

void cpu_t::check_interrupts()
{
    if(!IME)
        return;
//  std::cout << "IME: " << IME << " IE: " << std::hex << (unsigned int)*IE << " IF: " << std::hex << (unsigned int)*IF << std::endl;

    reg8 flags = *IE & *IF;
    if(flags)
    {
        IME = false;
        halted = false;
        if(flags & FLAG_I_VBLANK)
        {
            std::cout << "VBLANK" << std::endl;
            *IF &= ~FLAG_I_VBLANK;
            call(0x40);
        }

        if(flags & FLAG_I_LCDSTAT)
        {
            *IF &= ~FLAG_I_LCDSTAT;
            call(0x48);
        }

        if(flags & FLAG_I_TIMER)
        {
            *IF &= ~FLAG_I_TIMER;
            call(0x50);
        }

        if(flags & FLAG_I_SERIAL)
        {
            *IF &= ~FLAG_I_SERIAL;
            call(0x58);
        }

        if(flags & FLAG_I_JOYPAD)
        {
            *IF &= ~FLAG_I_JOYPAD;
            call(0x60);
        }
    }
}

void cpu_t::inc_counters()
{
    reg8 *ly = membus->get_pointer(0xFF44);
    if(*ly == 0x90)
    {
        //std::cout << "VBLANK" << std::endl;
        *IF |= FLAG_I_VBLANK;
    }
    *ly += 1;

    // std::cout << (std::hex) << (unsigned int) *ly << std::endl;
/*  if(*ly > 0x99)
    {
    //  *ly = 0;    // sad hack is sad :(
        *ly = 0x90;
    }*/
}

reg8 *cpu_t::get_reg(reg8_e reg)
{
    switch(reg)
    {
        case A:     return &(registers[AF].r8.h);
        case F:     return &(registers[AF].r8.l);
        case B:     return &(registers[BC].r8.h);
        case C:     return &(registers[BC].r8.l);
        case D:     return &(registers[DE].r8.h);
        case E:     return &(registers[DE].r8.l);
        case H:     return &(registers[HL].r8.h);
        case L:     return &(registers[HL].r8.l);
        default:    return 0x00;    // Crash!
    }
}

reg16 *cpu_t::get_reg(reg16_e reg)
{
    switch(reg)
    {
        case AF:    return &(registers[AF].r16);
        case BC:    return &(registers[BC].r16);
        case DE:    return &(registers[DE].r16);
        case HL:    return &(registers[HL].r16);
        case SP:    return &(registers[SP].r16);
        case PC:
        default:    return &(registers[PC].r16);
    }
}

reg8 cpu_t::read_reg(reg8_e reg)
{
    switch(reg)
    {
        case A:     return (registers[AF].r8.h);
        case F:     return (registers[AF].r8.l);
        case B:     return (registers[BC].r8.h);
        case C:     return (registers[BC].r8.l);
        case D:     return (registers[DE].r8.h);
        case E:     return (registers[DE].r8.l);
        case H:     return (registers[HL].r8.h);
        case L:     return (registers[HL].r8.l);
        case _HL_:  return membus->read(*get_reg(HL));
        default:    return 0x00;    // Crash!
    }
}

reg8_e cpu_t::get_r(const reg8 r)
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
reg16_e cpu_t::get_rp(const reg8 r)
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

reg16_e cpu_t::get_rp2(const reg8 r)
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

cond_e cpu_t::get_cc(const reg8 r)
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

alu_e cpu_t::get_alu(const reg8 r)
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

rot_e cpu_t::get_rot(const reg8 r)
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

void cpu_t::add(const reg8 src)
{
    reg8 result = *get_reg(A) + src;
    if(result == 0x00)
        *get_reg(F) |= FLAG_Z;
    else
        *get_reg(F) &= ~FLAG_Z;
    *get_reg(F) &= ~FLAG_N;
    if((*get_reg(A) & 0x08) && (src & 0x08))
        *get_reg(F) |= FLAG_H;
    if((*get_reg(A) & 0x80) && (src & 0x80))
        *get_reg(F) |= FLAG_C;
    *get_reg(A) = result;
}

void cpu_t::adc(const reg8 src)
{
    int result = *get_reg(A) + src;
    if(*get_reg(F) & FLAG_C)
        result += 0x01;
    if((result & 0xFF) == 0)
        *get_reg(F) |= FLAG_Z;
    else
        *get_reg(F) &= ~FLAG_Z;
    *get_reg(F) &= ~FLAG_N;
    if((*get_reg(A) & 0x08) && (src & 0x08))
        *get_reg(F) |= FLAG_H;
    else
        *get_reg(F) &= ~FLAG_H;
    if(result > 0xFF)
        *get_reg(F) |= FLAG_C;
    else
        *get_reg(F) &= ~FLAG_C;
    *get_reg(A) = result;
}

void cpu_t::_and(const reg8 src)
{
    *get_reg(A) &= src;
    if(*get_reg(A) == 0x00)
        *get_reg(F) |= FLAG_Z;
    else
        *get_reg(F) &= ~FLAG_Z;
    *get_reg(F) &= ~FLAG_N;
    *get_reg(F) |= FLAG_H;
    *get_reg(F) &= ~FLAG_C;
}

void cpu_t::cp(const reg8 src)
{
    reg8 result = *get_reg(A);
    sub(src);   // sets right flags
    *get_reg(A) = result;
}

void cpu_t::_or(const reg8 src)
{
    *get_reg(A) |= src;
    if(*get_reg(A) == 0x00)
        *get_reg(F) |= FLAG_Z;
    else
        *get_reg(F) &= ~FLAG_Z;
    *get_reg(F) &= ~FLAG_N;
    *get_reg(F) &= ~FLAG_H;
    *get_reg(F) &= ~FLAG_Z;
}

void cpu_t::sub(const reg8 src)
{
    reg8 result = *get_reg(A) - src;
//  if(*get_reg(F) & FLAG_C)
//      result -= 0x80;
    if((result ) == 0x00)
        *get_reg(F) |= FLAG_Z;
    else
        *get_reg(F) &= ~FLAG_Z;

    *get_reg(F) |= FLAG_N;
    if((*get_reg(A) & 0x10) || !((*get_reg(A) & 0x10) && src))
        *get_reg(F) |= FLAG_H;
    else
        *get_reg(F) &= ~FLAG_H;
    if(result > *get_reg(A))
        *get_reg(F) |= FLAG_C;
    else
        *get_reg(F) &= ~FLAG_C;
    *get_reg(A) = result;
}

void cpu_t::sbc(const reg8 src)
{
    reg8 result = *get_reg(A) - src;
    if(result == 0x00)
        *get_reg(F) |= FLAG_Z;
    else
        *get_reg(F) &= ~FLAG_Z;
    *get_reg(F) |= FLAG_N;
    if((*get_reg(A) & 0x10) || !((*get_reg(A) & 0x10) && src))
        *get_reg(F) |= FLAG_H;
    else
        *get_reg(F) &= ~FLAG_H;
    if(!(*get_reg(A) & 0x80) && ((src & 0x80)
                                    || (*get_reg(F) & FLAG_C)))
        *get_reg(F) |= FLAG_C;
    else
        *get_reg(F) &= ~FLAG_C;
    *get_reg(A) = result;
}

void cpu_t::_xor(const reg8 src)
{
    *get_reg(A) ^= src;
    if(*get_reg(A) == 0x00)
        *get_reg(F) |= FLAG_Z;
    else
        *get_reg(F) &= ~FLAG_Z;
    *get_reg(F) &= ~FLAG_N;
    *get_reg(F) &= ~FLAG_H;
    *get_reg(F) &= ~FLAG_Z;
}

void cpu_t::add(const reg8_e src) { add(read_reg(src));};
void cpu_t::adc(const reg8_e src) { adc(read_reg(src));};
void cpu_t::_and(const reg8_e src){_and(read_reg(src));};
void cpu_t::cp(const reg8_e src){   cp(read_reg(src));};
void cpu_t::_or(const reg8_e src){ _or(read_reg(src));};
void cpu_t::sub(const reg8_e src){  sub(read_reg(src));};
void cpu_t::sbc(const reg8_e src){  sbc(read_reg(src));};
void cpu_t::_xor(const reg8_e src){_xor(read_reg(src));};

void cpu_t::dec(const reg8_e dest)
{
    reg8 result =
        (dest == _HL_ ? membus->read(*get_reg(HL)) : (*get_reg(dest)));
    result--;

    if((result & 0xFF) == 0x00)
        *get_reg(F) |= FLAG_Z;
    else
        *get_reg(F) &= ~FLAG_Z;

    *get_reg(F) |= FLAG_N;
    if((result & 0x0F) == 0x0F)
        *get_reg(F) |= FLAG_H;
    else
        *get_reg(F) &= ~FLAG_H;

    if(dest == _HL_)
        membus->write(*get_reg(HL), result);
    else
        *get_reg(dest) = result;
}

void cpu_t::inc(const reg8_e dest)
{
    reg8 result =
        (dest == _HL_ ? membus->read(*get_reg(HL)) : (*get_reg(dest)));
    result++;

    if(result == 0x00)
        *get_reg(F) |= FLAG_Z;
    else
        *get_reg(F) &= ~FLAG_Z;

    *get_reg(F) &= ~FLAG_N;
    if(*get_reg(dest) & 0x08)
        *get_reg(F) |= FLAG_H;
    else
        *get_reg(F) &= ~FLAG_H;

    if(dest == _HL_)
        membus->write(*get_reg(HL), result);
    else
        *get_reg(dest) = result;
}

void cpu_t::swap(const reg8_e dest)
{
    reg8 temp = 0x00;
    if((*get_reg(dest) & 0x80) == 0x80)
        temp |= 0x01;
    if((*get_reg(dest) & 0x01) == 0x01)
        temp |= 0x08;

    *get_reg(dest) &= ~0x81;
    *get_reg(dest) |= temp;

    if((*get_reg(dest) & 0xFF) == 0x00)
        *get_reg(F) |= FLAG_Z;
    else
        *get_reg(F) &= ~FLAG_Z;
    *get_reg(F) &= ~FLAG_N;
    *get_reg(F) &= ~FLAG_H;
    *get_reg(F) &= ~FLAG_C;
}

void cpu_t::swaphl()
{
    reg8 temp = 0x00;
    reg8 data = membus->read(*get_reg(HL));
    if((data & 0x80) == 0x80)
        temp |= 0x01;
    if((data & 0x01) == 0x01)
        temp |= 0x08;

    data &= ~0x81;
    data |= temp;
    membus->write(*get_reg(HL), data);

    if((data & 0xFF) == 0x00)
        *get_reg(F) |= FLAG_Z;
    else
        *get_reg(F) &= ~FLAG_Z;
    *get_reg(F) &= ~FLAG_N;
    *get_reg(F) &= ~FLAG_H;
    *get_reg(F) &= ~FLAG_C;
}

void cpu_t::daa()
{
    reg8 result = *get_reg(A);
    reg8 acc_hi = result & 0x0F;
    reg8 acc_lo = result & 0xF0;
    reg8 f_n = *get_reg(F) & FLAG_N;
    reg8 f_c = *get_reg(F) & FLAG_C;
    reg8 f_h = *get_reg(F) & FLAG_H;
    reg8 new_flags = 0x00;
    if((f_n | f_c) == 0x00)
    {
        if(!f_h)
        {
            if(acc_hi < 0x0A && acc_lo < 0xA0)
            {
                result += 0x00;
            }
            else if(acc_hi < 0x09 && acc_lo > 0x90)
            {
                result += 0x06;
            }
            else if(acc_hi > 0x09 && acc_lo < 0xA0)
            {
                result += 0x60;
                new_flags |= FLAG_C;
            }
            else if(acc_hi > 0x08 && acc_lo > 0x90)
            {
                result += 0x66;
                new_flags |= FLAG_C;
            }
        }
        else if(f_h)
        {
            if(acc_hi < 0x0A && acc_lo < 0x40)
            {
                result += 0x06;
            }
            else if(acc_hi > 0x09 && acc_lo < 0x40)
            {
                result += 0x60;
                new_flags |= FLAG_C;
            }
        }
    }
    else if((f_n | f_c) == FLAG_C)
    {
        if(!f_h && acc_hi < 0x03 && acc_lo < 0xA0)
        {
            result += 0x60;
            new_flags |= FLAG_C;
        }
        else if(!f_h && acc_hi < 0x03 && acc_lo > 0x90)
        {
            result += 0x66;
            new_flags |= FLAG_C;
        }
        else if(f_h && acc_hi < 0x03 && acc_lo < 0x3)
        {
            result += 0x66;
            new_flags |= FLAG_C;
        }
    }
    else if((f_n | f_c) == FLAG_N)
    {
        if(!f_h && acc_hi < 0x0A && acc_lo < 0xA0)
        {
            result += 0x00;
        }
        if(f_h && acc_hi < 0x09 && acc_lo > 0x50)
        {
            result += 0xFA;
        }
    }
    else if((f_n | f_c) == (FLAG_N & FLAG_C))
    {
        if(!f_h && acc_hi > 0x06 && acc_lo < 0xA0)
        {
            result += 0xA0;
            new_flags |= FLAG_C;
        }
        else if(f_h && acc_hi > 0x05 && acc_lo > 0x50)
        {
            result += 0x9A;
            new_flags |= FLAG_C;
        }
    }
    if(result == 0x00)
        new_flags |= FLAG_Z;
    else
        new_flags &= ~FLAG_Z;
    if(f_n)
        new_flags |= FLAG_N;
    else
        new_flags &= ~FLAG_N;
    *get_reg(A) = result;
    *get_reg(F) = new_flags;
}

void cpu_t::cpl()
{
    *get_reg(A) = *get_reg(A);
    *get_reg(F) |= FLAG_N;
    *get_reg(F) |= FLAG_H;
}

void cpu_t::ccf()
{
    if((*get_reg(F) & FLAG_C) == FLAG_C)
        *get_reg(F) &= ~FLAG_C;
    else
        *get_reg(F) |= FLAG_C;

    *get_reg(F) &= ~FLAG_N;
    *get_reg(F) &= ~FLAG_H;
}

void cpu_t::scf()
{

    *get_reg(F) |= FLAG_C;
    *get_reg(F) &= ~FLAG_N;
    *get_reg(F) &= ~FLAG_H;
}

void cpu_t::nop()
{
//  panic();
}

void cpu_t::halt()
{
    std::cout << "Halt\n";
    halted = true;
}

void cpu_t::stop()
{
    std::cout << "STOP!\n";
    panic();
}

void cpu_t::di()
{
    IME = false;
}

void cpu_t::ei()
{
    IME = true;
}

void cpu_t::ld(const reg8_e dest, const reg8_e src)
{
    switch(dest)
    {
        case _HL_: membus->write(*get_reg(HL), *get_reg(src));          return;
        case _C_:  membus->write(0xFF00 + *get_reg(C), *get_reg(src));  return;
        case _BC_: membus->write(*get_reg(BC), *get_reg(src));          return;
        case _DE_: membus->write(*get_reg(DE), *get_reg(src));          return;
        default: break;
    }
    switch(src)
    {
        case _HL_: *get_reg(dest) = membus->read(*get_reg(HL));         return;
        case _C_:  *get_reg(dest) = membus->read(0xFF00 + *get_reg(C)); return;
        case _BC_: *get_reg(dest) = membus->read(*get_reg(BC));         return;
        case _DE_: *get_reg(dest) = membus->read(*get_reg(DE));         return;
        default: break;
    }

    *get_reg(dest) = *get_reg(src);
}

void cpu_t::ld(const reg8_e dest, const reg8 src)
{
    switch(dest)
    {
        case _HL_: membus->write(*get_reg(HL), src);            return;
        case _C_:  membus->write(0xFF00 + *get_reg(C), src);    return;
        case _BC_: membus->write(*get_reg(BC), src);            return;
        case _DE_: membus->write(*get_reg(DE), src);            return;
        default: break;
    }

    *get_reg(dest) = src;
}

void cpu_t::ld(const reg16_e dest, const reg16_2x8 src)
{
    *get_reg(dest) = src.r16;
}

void cpu_t::ldsphl()
{
    *get_reg(SP) = *get_reg(HL);
}

void cpu_t::ldabc()
{
    *get_reg(A) = membus->read(*get_reg(BC));
}
void cpu_t::ldbca()
{
    membus->write(*get_reg(BC), *get_reg(A));
}
void cpu_t::ldade()
{
    *get_reg(A) = membus->read(*get_reg(DE));
}
void cpu_t::lddea()
{
    membus->write(*get_reg(DE), *get_reg(A));
}
void cpu_t::ldahl()
{
    *get_reg(A) = membus->read(*get_reg(HL));
}
void cpu_t::ldhla()
{
    membus->write(*get_reg(HL), *get_reg(A));
}

void cpu_t::lddahl()
{
    *get_reg(A) = membus->read((*get_reg(HL))--);
}
void cpu_t::lddhla()
{
    membus->write((*get_reg(HL))--, *get_reg(A));
}

void cpu_t::ldiahl()
{
    *get_reg(A) = membus->read((*get_reg(HL))++);
}

void cpu_t::ldihla()
{
    //std::cout << "PC: " << std::hex << (int)*get_reg(PC) << "\n";
    membus->write((*get_reg(HL))++, *get_reg(A));
}

void cpu_t::ldhna_byte(const int8_t n)
{
    ldhna_word(0xFF00 + n);
}

void cpu_t::ldhan_byte(const int8_t n)
{
    ldhan_word(0xFF00 + n);
}

void cpu_t::ldhlspn_byte(const int8_t n)
{
    *get_reg(A) = membus->read(*get_reg(SP) + n);
}

void cpu_t::ldhna_word(const reg16 n)
{
    membus->write(n, *get_reg(A));
}

void cpu_t::ldhan_word(const reg16 n)
{
    *get_reg(A) = membus->read(n);
}

void cpu_t::ldhnnsp(const reg16 n)
{
    membus->write(n, *get_reg(SP));
}

void cpu_t::ldhspnn(const reg16 n)
{
    *get_reg(A) = membus->read(n);
}

void cpu_t::rlca()
{
    rlc(A);
}

void cpu_t::rlchl()
{
    reg8 adata = *get_reg(A);
    ldahl();
    rlc(A);
    ldhla();
    *get_reg(A) = adata;
}

void cpu_t::rlhl()
{
    reg8 adata = *get_reg(A);
    ldahl();
    rl(A);
    ldhla();
    *get_reg(A) = adata;
}

void cpu_t::rrchl()
{
    reg8 adata = *get_reg(A);
    ldahl();
    rrc(A);
    ldhla();
    *get_reg(A) = adata;
}

void cpu_t::rrhl()
{
    reg8 adata = *get_reg(A);
    ldahl();
    rr(A);
    ldhla();
    *get_reg(A) = adata;
}

void cpu_t::slahl()
{
    reg8 adata = *get_reg(A);
    ldahl();
    sla(A);
    ldhla();
    *get_reg(A) = adata;
}

void cpu_t::srahl()
{
    reg8 adata = *get_reg(A);
    ldahl();
    sra(A);
    ldhla();
    *get_reg(A) = adata;
}

void cpu_t::srlhl()
{
    reg8 adata = *get_reg(A);
    ldahl();
    srl(A);
    ldhla();
    *get_reg(A) = adata;
}

void cpu_t::rlc(const reg8_e dest)
{
    reg8 cdata = (*get_reg(dest) & 0x80) ? 0x01 : 0x00;
    if(cdata)
        *get_reg(F) |= FLAG_C;

    *get_reg(dest) = *get_reg(dest) << 1;
    *get_reg(dest) |= cdata;

    if(*get_reg(dest) == 0x00)
        *get_reg(F) |= FLAG_Z;
    else
        *get_reg(F) &= ~FLAG_Z;
    *get_reg(F) &= ~FLAG_N;
    *get_reg(F) &= ~FLAG_H;
}

void cpu_t::rla()
{
    rl(A);
}

void cpu_t::rl(const reg8_e dest)
{
    reg8 cdata = (*get_reg(F) & FLAG_C) ? 0x01 : 0x00;
    if(*get_reg(dest) & 0x80)
        *get_reg(F) |= FLAG_C;
    else
        *get_reg(F) &= ~FLAG_C;

    *get_reg(dest) = *get_reg(dest) << 1;
    *get_reg(dest) |= cdata;

    if(*get_reg(dest) == 0x00)
        *get_reg(F) |= FLAG_Z;
    else
        *get_reg(F) &= ~FLAG_Z;

    *get_reg(F) &= ~FLAG_N;
    *get_reg(F) &= ~FLAG_H;
}

void cpu_t::rrca()
{
    rrc(A);
}

void cpu_t::rrc(const reg8_e dest)
{
    reg8 cdata = (*get_reg(dest) & 0x01) ? 0x80 : 0x00;
    if(cdata)
        *get_reg(F) |= FLAG_C;
    else
        *get_reg(F) &= ~FLAG_C;

    *get_reg(dest) = *get_reg(dest) >> 1;
    *get_reg(dest) |= cdata;

    if(*get_reg(dest) == 0x00)
        *get_reg(F) |= FLAG_Z;
    else
        *get_reg(F) &= ~FLAG_Z;

    *get_reg(F) &= ~FLAG_N;
    *get_reg(F) &= ~FLAG_H;
}

void cpu_t::rra()
{
    rr(A);
}

void cpu_t::rr(const reg8_e dest)
{
    reg8 cdata = (*get_reg(F) & FLAG_C) ? 0x80 : 0x00;
    if(*get_reg(dest) & 0x01)
        *get_reg(F) |= FLAG_C;
    else
        *get_reg(F) &= ~FLAG_C;

    *get_reg(dest) = *get_reg(dest) >> 1;
    *get_reg(dest) |= cdata;

    if(*get_reg(dest) == 0x00)
        *get_reg(F) |= FLAG_Z;
    else
        *get_reg(F) &= ~FLAG_Z;
    *get_reg(F) &= ~FLAG_N;
    *get_reg(F) &= ~FLAG_H;
}

void cpu_t::sla(const reg8_e dest)
{
    if(*get_reg(dest) & 0x80)
        *get_reg(F) |= FLAG_C;
    else
        *get_reg(F) &= ~FLAG_C;

    *get_reg(dest) = *get_reg(dest) << 1;
    if(*get_reg(dest) == 0x00)
        *get_reg(F) |= FLAG_Z;
    else
        *get_reg(F) &= ~FLAG_Z;
    *get_reg(F) &= ~FLAG_N;
    *get_reg(F) &= ~FLAG_H;
}

void cpu_t::sra(const reg8_e dest)
{
    reg8 msbdata = *get_reg(dest) & 0x80;
    if(*get_reg(dest) & 0x01)
        *get_reg(F) |= FLAG_C;
    else
        *get_reg(F) &= ~FLAG_C;

    *get_reg(dest) = *get_reg(dest) >> 1;
    *get_reg(dest) |= msbdata;

    if(*get_reg(dest) == 0x00)
        *get_reg(F) |= FLAG_Z;
    else
        *get_reg(F) &= ~FLAG_Z;
    *get_reg(F) &= ~FLAG_N;
    *get_reg(F) &= ~FLAG_H;
}

void cpu_t::srl(const reg8_e dest)
{
    if(*get_reg(dest) & 0x01)
        *get_reg(F) |= FLAG_C;
    else
        *get_reg(F) &= ~FLAG_C;

    *get_reg(dest) = *get_reg(dest) >> 1;

    if(*get_reg(dest) == 0x00)
        *get_reg(F) |= FLAG_Z;
    else
        *get_reg(F) &= ~FLAG_Z;
    *get_reg(F) &= ~FLAG_N;
    *get_reg(F) &= ~FLAG_H;
}

void cpu_t::addhl(reg16_e src)
{
    reg16 result = *get_reg(HL) + *get_reg(src);
    *get_reg(F) &= ~FLAG_N;
    if((*get_reg(HL) & 0x0800) && (*get_reg(src) & 0x0800))
        *get_reg(F) |= FLAG_H;
    else
        *get_reg(F) &= ~FLAG_H;
    if((*get_reg(HL) & 0x8000) && (*get_reg(src) & 0x8000))
        *get_reg(F) |= FLAG_C;
    else
        *get_reg(F) &= ~FLAG_C;
    *get_reg(HL) = result;
}

void cpu_t::addsp(const reg8 src)
{
    reg16 result = *get_reg(SP) + src;
    *get_reg(F) &= ~FLAG_Z;
    *get_reg(F) &= ~FLAG_N;
    if((*get_reg(SP) & 0x0800) && (src & 0x0800))
        *get_reg(F) |= FLAG_H;
    else
        *get_reg(F) &= ~FLAG_H;
    if((*get_reg(SP) & 0x8000) && (src & 0x8000))
        *get_reg(F) |= FLAG_C;
    else
        *get_reg(F) &= ~FLAG_C;
    *get_reg(SP) = result;
}

void cpu_t::inc(reg16_e dest)
{
    (*get_reg(dest))++;
}
void cpu_t::dec(reg16_e dest)
{
    (*get_reg(dest))--;
}

void cpu_t::bit(const reg8 b, const reg8_e src)
{
    reg8 val;
    if(src == _HL_)
        val = membus->read(*get_reg(HL));
    else
        val = *get_reg(src);

    unsigned char c = 0x00;
    switch(b)
    {
        case 0: c = 0x01;   break;
        case 1: c = 0x02;   break;
        case 2: c = 0x04;   break;
        case 3: c = 0x08;   break;
        case 4: c = 0x10;   break;
        case 5: c = 0x20;   break;
        case 6: c = 0x40;   break;
        case 7: c = 0x80;   break;
    }
    if(((val & c) & 0xFF) == 0x00)
        *get_reg(F) |= FLAG_Z;
    else
        *get_reg(F) &= ~FLAG_Z;
    *get_reg(F) &= ~FLAG_N;
    *get_reg(F) |= FLAG_H;
}

void cpu_t::set(const reg8 b, const reg8_e src)
{
    unsigned char c = 0x00;
    switch(b)
    {
        case 0: c = 0x01;   break;
        case 1: c = 0x02;   break;
        case 2: c = 0x04;   break;
        case 3: c = 0x08;   break;
        case 4: c = 0x10;   break;
        case 5: c = 0x20;   break;
        case 6: c = 0x40;   break;
        case 7: c = 0x80;   break;
    }

    if(src == _HL_)
        membus->write(*get_reg(HL), (membus->read(*get_reg(HL)) | c));
    else
        *get_reg(src) |= c;
}

void cpu_t::res(const reg8 b, const reg8_e src)
{
    unsigned char c = 0x00;
    switch(b)
    {
        case 0: c = 0x01;   break;
        case 1: c = 0x02;   break;
        case 2: c = 0x04;   break;
        case 3: c = 0x08;   break;
        case 4: c = 0x10;   break;
        case 5: c = 0x20;   break;
        case 6: c = 0x40;   break;
        case 7: c = 0x80;   break;
    }

    if(src == _HL_)
        membus->write(*get_reg(HL), (membus->read(*get_reg(HL)) & ~c));
    else
        *get_reg(src) &= ~c;
}

#define C_NZ    ((*get_reg(F) & FLAG_Z) == 0x00)
#define C_Z     ((*get_reg(F) & FLAG_Z) == FLAG_Z)
#define C_NC    ((*get_reg(F) & FLAG_C) == 0x00)
#define C_CC    (*get_reg(F) & FLAG_C)

void cpu_t::jp(const cond_e c, const uint16_t d)
{
    switch(c)
    {
        case NZ:    if(C_NZ)    jp(d);  return;
        case Z:     if(C_Z)     jp(d);  return;
        case NC:    if(C_NC)    jp(d);  return;
        case CC:    if(C_CC)    jp(d);  return;
        case PO:
        case PE:
        case P:
        case M:
        default:    panic();            return;
    }
}

void cpu_t::jp(const uint16_t d)
{
    *get_reg(PC) = d;
}

void cpu_t::jphl()
{
    jp(*get_reg(HL));
}

void cpu_t::jr(const cond_e c, const int8_t d)
{
    switch(c)
    {
        case NZ:    if(C_NZ)    jr(d);  return;
        case Z:     if(C_Z)     jr(d);  return;
        case NC:    if(C_NC)    jr(d);  return;
        case CC:    if(C_CC)    jr(d);  return;
        case PO:
        case PE:
        case P:
        case M:
        default:    panic();            return;
    }
}

void cpu_t::jr(const int8_t d)
{
    *get_reg(PC) += d;
}

void cpu_t::push(const reg16_e nn)
{
    reg16_2x8 data;
    data.r16 = *get_reg(nn);

    membus->write(--(*get_reg(SP)), data.r8.h);
    membus->write(--(*get_reg(SP)), data.r8.l);

}

void cpu_t::pop(const reg16_e nn)
{
    reg16_2x8 data;
    data.r8.l = membus->read((*get_reg(SP))++);
    data.r8.h = membus->read((*get_reg(SP))++);

    *get_reg(nn) = data.r16;
}

void cpu_t::call(const reg16 nn)
{
    push(PC);
    *get_reg(PC) = nn;
}

void cpu_t::call(const cond_e c, const reg16 nn)
{
    switch(c)
    {
        case NZ:    if(C_NZ)    call(nn);   return;
        case Z:     if(C_Z)     call(nn);   return;
        case NC:    if(C_NC)    call(nn);   return;
        case CC:    if(C_CC)    call(nn);   return;
        case PO:
        case PE:
        case P:
        case M:
        default:    panic();                return;
    }
}


void cpu_t::rst(const reg8 n)
{
    push(PC);
    jp(0x0000 + n);
}

void cpu_t::ret()
{
    pop(PC);
}

void cpu_t::ret(const cond_e c)
{
    switch(c)
    {
        case NZ:    if(C_NZ)    ret();  return;
        case Z:     if(C_Z)     ret();  return;
        case NC:    if(C_NC)    ret();  return;
        case CC:    if(C_CC)    ret();  return;
        case PO:
        case PE:
        case P:
        case M:
        default:    panic();            return;
    }
}

void cpu_t::reti()
{
    ret();
    IME = true;
}

std::string reg8_e_tostring(const reg8_e r)
{
    switch(r)
    {
        case A: return "A";
        case B: return "B";
        case C: return "C";
        case D: return "D";
        case E: return "E";
        case F: return "F";
        case H: return "H";
        case L: return "L";
        case _HL_:return "(HL)";
        default:return "?";
    }
}

std::string reg16_e_tostring(const reg16_e r)
{
    switch(r)
    {
        case AF: return "AF";
        case BC: return "BC";
        case DE: return "DE";
        case HL: return "HL";
        case SP: return "SP";
        case PC: return "PC";
        default: return "??";
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

std::string binstring(const unsigned char byte)
{
    std::string s;
    int i, j;
    for(i = 0, j = 0x80; i < 8; ++i)
    {
        s += (byte & j ? '1' : '0');
        j >>= 1;
    }
    return s;
}

std::string binstring(const unsigned short bytes)
{
    std::string s;
    int i, j;
    for(i = 0, j = 0x8000; i < 16; ++i)
    {
        s += (bytes & j ? '1' : '0');
        j >>= 1;
    }
    return s;
}

void cpu_t::print()
{
    std::cout << "[Registers]\nR dec hex bin\n";
    for(int i = 0; i < 8; ++i)
    {
        reg8_e r = (reg8_e)i;
        std::cout << reg8_e_tostring(r) << " ";
        std::cout << std::setw(3) << (int)(*get_reg(r)) << ", ";
        std::cout << std::hex << std::setw(3) << (int)*get_reg(r);
        std::cout << std::dec << ", " << binstring(*get_reg(r)) << "\n";
    }
    for(int i = 0; i < 6; ++i)
    {
        reg16_e r = (reg16_e)i;
        std::cout << reg16_e_tostring(r) << " ";
        std::cout << std::setw(6) << (int)(*get_reg(r)) << ", ";
        std::cout << std::hex << std::setw(6) << (int)*get_reg(r);
        std::cout << "    (" << reg16_e_tostring(r) << ") ";
        std::cout << std::setw(6) << (int)membus->read(*get_reg(r)) << ", ";
        std::cout << std::hex << std::setw(6) << (int)membus->read(*get_reg(r));
        std::cout << std::dec << ", " << binstring(*get_reg(r)) << "\n";
    }
    std::cout << "[Flags]\nZNHC\n" << binstring(*get_reg(F)) << "\n";
}

void cpu_t::inject_code(uint8_t *code, size_t length, reg16 new_pc, int steps)
{
    // Disable bootrom to make sure code is injected in ROM, not BOOTROM.
    bool reenable_bootrom = !booted;
    membus->disable_bootrom();

    reg16 old_pc = *get_reg(PC);
    int8_t *old_code = new int8_t[length];

    memcpy(old_code, membus->get_pointer(new_pc), length);
    memcpy(membus->get_pointer(new_pc), code, length);

    *get_reg(PC) = new_pc;
    for(int i = 0; i < steps && !panicked; ++i)
    {
        run();
    }
    if(steps == 0 && !panicked)
    {
        do{run();}
        while(last_instr != 0x00 && !panicked);
    }

    memcpy(membus->get_pointer(old_pc), old_code, length);
    *get_reg(PC) = old_pc;
    delete old_code;

    if(reenable_bootrom)
        membus->enable_bootrom();
}

void cpu_t::panic()
{
    std::cout << "========\nCPU panicked!\n";
    std::cout << "Last instruction 0x" << std::hex << (int)last_instr
        << " at adr 0x" << std::hex << (int)last_adr << "\n";
    std::cout << "========\n";
    print();
    panicked = true;
}

void cpu_t::cycle(uint8_t n)
{
    cycles_left = n;
}

void cpu_t::set_flags(bool N, bool Z, bool H, bool C)
{
    *get_reg(F) = (N ? *get_reg(F) | FLAG_N : *get_reg(F) & ~FLAG_N);
    *get_reg(F) = (Z ? *get_reg(F) | FLAG_Z : *get_reg(F) & ~FLAG_Z);
    *get_reg(F) = (H ? *get_reg(F) | FLAG_H : *get_reg(F) & ~FLAG_H);
    *get_reg(F) = (C ? *get_reg(F) | FLAG_C : *get_reg(F) & ~FLAG_C);
}

bool cpu_t::is_panicked() const
{
    return panicked;
}
void cpu_t::id_execute()
{
    last_adr = *get_reg(PC);
    reg8 instr = read_mem();
    last_instr = instr;

    if(!booted && last_adr > 0xFF)
    {
        booted = true;
        membus->disable_bootrom();
    }

#if DEBUG_OUTPUT > 0
    if(!booted)
        std::cout << "[BOOT]";

    std::cout << "PC: " << std::hex << (int)last_adr << " INSTR: "
        << (int)last_instr << " ";
#endif
#if DEBUG_OUTPUT == 1
    std::cout << "\n";
#endif

    reg16_2x8 data16; data16.r16 = 0x00;
    reg8 data8 = 0x00;

    switch(instr)
    {
        case 0x06:  data8 = read_mem(); ld(B,  data8);   break;
        case 0x0E:  data8 = read_mem(); ld(C,  data8);   break;
        case 0x16:  data8 = read_mem(); ld(D,  data8);   break;
        case 0x1E:  data8 = read_mem(); ld(E,  data8);   break;
        case 0x26:  data8 = read_mem(); ld(H,  data8);   break;
        case 0x2E:  data8 = read_mem(); ld(L,  data8);   break;
        case 0x36:  data8 = read_mem(); ld(_HL_, data8); break;

        case 0x40:  ld(B, B);    break;
        case 0x41:  ld(B, C);    break;
        case 0x42:  ld(B, D);    break;
        case 0x43:  ld(B, E);    break;
        case 0x44:  ld(B, H);    break;
        case 0x45:  ld(B, L);    break;
        case 0x46:  ld(B, _HL_); break;

        case 0x48:  ld(C, B);    break;
        case 0x49:  ld(C, C);    break;
        case 0x4A:  ld(C, D);    break;
        case 0x4B:  ld(C, E);    break;
        case 0x4C:  ld(C, H);    break;
        case 0x4D:  ld(C, L);    break;
        case 0x4E:  ld(C, _HL_); break;

        case 0x50:  ld(D, B);    break;
        case 0x51:  ld(D, C);    break;
        case 0x52:  ld(D, D);    break;
        case 0x53:  ld(D, E);    break;
        case 0x54:  ld(D, H);    break;
        case 0x55:  ld(D, L);    break;
        case 0x56:  ld(D, _HL_); break;

        case 0x58:  ld(E, B);    break;
        case 0x59:  ld(E, C);    break;
        case 0x5A:  ld(E, D);    break;
        case 0x5B:  ld(E, E);    break;
        case 0x5C:  ld(E, H);    break;
        case 0x5D:  ld(E, L);    break;
        case 0x5E:  ld(E, _HL_); break;

        case 0x60:  ld(H, B);    break;
        case 0x61:  ld(H, C);    break;
        case 0x62:  ld(H, D);    break;
        case 0x63:  ld(H, E);    break;
        case 0x64:  ld(H, H);    break;
        case 0x65:  ld(H, L);    break;
        case 0x66:  ld(H, _HL_); break;

        case 0x68:  ld(L, B);    break;
        case 0x69:  ld(L, C);    break;
        case 0x6A:  ld(L, D);    break;
        case 0x6B:  ld(L, E);    break;
        case 0x6C:  ld(L, H);    break;
        case 0x6D:  ld(L, L);    break;
        case 0x6E:  ld(L, _HL_); break;

        case 0x70:  ld(_HL_, B);   break;
        case 0x71:  ld(_HL_, C);   break;
        case 0x72:  ld(_HL_, D);   break;
        case 0x73:  ld(_HL_, E);   break;
        case 0x74:  ld(_HL_, H);   break;
        case 0x75:  ld(_HL_, L);   break;

        case 0x7F:  ld(A, A);    break;
        case 0x78:  ld(A, B);    break;
        case 0x79:  ld(A, C);    break;
        case 0x7A:  ld(A, D);    break;
        case 0x7B:  ld(A, E);    break;
        case 0x7C:  ld(A, H);    break;
        case 0x7D:  ld(A, L);    break;

        case 0x0A:  ldabc();    break;
        case 0x1A:  ldade();    break;
        case 0x7E:  ldahl();    break;
        case 0xFA:  data16.r8.l = read_mem(); data16.r8.h = read_mem(); ldhan_word(data16.r16); break;
        case 0x3E:  data8 = read_mem(); ld(A, data8);           break;

        case 0x47:  ld(B, A);   break;
        case 0x4F:  ld(C, A);   break;
        case 0x57:  ld(D, A);   break;
        case 0x5F:  ld(E, A);   break;
        case 0x67:  ld(H, A);   break;
        case 0x6F:  ld(L, A);   break;
        case 0x02:  ldbca();    break;
        case 0x12:  lddea();    break;
        case 0x77:  ldhla();    break;
        case 0xEA:  data16.r8.l = read_mem(); data16.r8.h = read_mem(); ldhna_word(data16.r16);break;

        case 0xF2:  ld(A, _C_); break;
        case 0xE2:  ld(_C_, A); break;

        case 0x3A:  lddahl();   break;
        case 0x32:  lddhla();   break;
        case 0x2A:  ldiahl();   break;
        case 0x22:  ldihla();   break;

        case 0xE0:  data8 = read_mem(); ldhna_byte(data8);     break;
        case 0xF0:  data8 = read_mem(); ldhan_byte(data8);     break;

        case 0x01:  data16.r8.l = read_mem(); data16.r8.h = read_mem(); ld(BC, data16);         break;
        case 0x11:  data16.r8.l = read_mem(); data16.r8.h = read_mem(); ld(DE, data16);         break;
        case 0x21:  data16.r8.l = read_mem(); data16.r8.h = read_mem(); ld(HL, data16);         break;
        case 0x31:  data16.r8.l = read_mem(); data16.r8.h = read_mem(); ld(SP, data16);         break;
        case 0xF9:  ldsphl();                                                                   break;
        case 0xF8:  data8 = read_mem(); ldhlspn_byte(data8);                                    break;
        case 0x08:  data16.r8.l = read_mem(); data16.r8.h = read_mem(); ldhnnsp(data16.r16);    break;

        case 0xF5:  push(AF);          break;
        case 0xC5:  push(BC);          break;
        case 0xD5:  push(DE);          break;
        case 0xE5:  push(HL);          break;

        case 0xF1:  pop(AF);           break;
        case 0xC1:  pop(BC);           break;
        case 0xD1:  pop(DE);           break;
        case 0xE1:  pop(HL);           break;

        case 0x87:  add(A);   break;
        case 0x80:  add(B);   break;
        case 0x81:  add(C);   break;
        case 0x82:  add(D);   break;
        case 0x83:  add(E);   break;
        case 0x84:  add(H);   break;
        case 0x85:  add(L);   break;
        case 0x86:  add(_HL_);break;
        case 0xC6:  data8 = read_mem(); add(data8);          break;

        case 0x8F:  adc(A);   break;
        case 0x88:  adc(B);   break;
        case 0x89:  adc(C);   break;
        case 0x8A:  adc(D);   break;
        case 0x8B:  adc(E);   break;
        case 0x8C:  adc(H);   break;
        case 0x8D:  adc(L);   break;
        case 0x8E:  adc(_HL_);break;
        case 0xCE:  data8 = read_mem(); adc(data8);          break;

        case 0x97:  sub(A);   break;
        case 0x90:  sub(B);   break;
        case 0x91:  sub(C);   break;
        case 0x92:  sub(D);   break;
        case 0x93:  sub(E);   break;
        case 0x94:  sub(H);   break;
        case 0x95:  sub(L);   break;
        case 0x96:  sub(_HL_);break;
        case 0xD6:  data8 = read_mem(); sub(data8);          break; /* Opcode unconfirmed */

        case 0x9F:  sbc(A);   break;
        case 0x98:  sbc(B);   break;
        case 0x99:  sbc(C);   break;
        case 0x9A:  sbc(D);   break;
        case 0x9B:  sbc(E);   break;
        case 0x9C:  sbc(H);   break;
        case 0x9D:  sbc(L);   break;
        case 0x9E:  sbc(_HL_);break;
        case 0xDE:  data8 = read_mem(); sbc(data8);          break; /* Opcode unconfirmed */


        case 0xAF:  _xor(A);   break;

        case 0xBF:  cp(A);              break;
        case 0xB8:  cp(B);              break;
        case 0xB9:  cp(C);              break;
        case 0xBA:  cp(D);              break;
        case 0xBB:  cp(E);              break;
        case 0xBC:  cp(H);              break;
        case 0xBD:  cp(L);              break;
        case 0xBE:  cp(_HL_);           break;
        case 0xFE:  data8 = read_mem(); cp(data8);                   break;

        case 0x3C:  inc(A);            break;
        case 0x04:  inc(B);            break;
        case 0x0C:  inc(C);            break;
        case 0x14:  inc(D);            break;
        case 0x1C:  inc(E);            break;
        case 0x24:  inc(H);            break;
        case 0x2C:  inc(L);            break;
        case 0x34:  inc(_HL_);         break;

        case 0x3D:  dec(A);            break;
        case 0x05:  dec(B);            break;
        case 0x0D:  dec(C);            break;
        case 0x15:  dec(D);            break;
        case 0x1D:  dec(E);            break;
        case 0x25:  dec(H);            break;
        case 0x2D:  dec(L);            break;
        case 0x35:  dec(_HL_);         break;

        case 0x03:  inc(BC);         break;
        case 0x13:  inc(DE);         break;
        case 0x23:  inc(HL);         break;
        case 0x33:  inc(SP);         break;

        case 0x0B:  dec(BC);         break;
        case 0x1B:  dec(DE);         break;
        case 0x2B:  dec(HL);         break;
        case 0x3B:  dec(SP);         break;

        case 0x27:  daa();          break;
        case 0x2F:  cpl();  break;
        case 0x37:  scf();  break;
        case 0x3F:  ccf();  break;
        case 0x00:  nop();  break;
        case 0x76:  halt(); break;
        case 0x10:  stop(); break; // TODO should be 0x10 0x00!
        case 0xF3:  di();   break;
        case 0xFB:  ei();   break;

        case 0x07:  rlca(); break;
        case 0x17:  rla();  break;
        case 0x0F:  rrca(); break;
        case 0x1F:  rra();  break;

        case 0xCB: /* Extended ALU Operations */
            instr = read_mem();
            switch(instr)
            {
                case 0x07:  rlc(A); break;
                case 0x00:  rlc(B); break;
                case 0x01:  rlc(C); break;
                case 0x02:  rlc(D); break;
                case 0x03:  rlc(E); break;
                case 0x04:  rlc(H); break;
                case 0x05:  rlc(L); break;
                case 0x06:  rlc(_HL_); break;

                case 0x17:  rl(A); break;
                case 0x10:  rl(B); break;
                case 0x11:  rl(C); break;
                case 0x12:  rl(D); break;
                case 0x13:  rl(E); break;
                case 0x14:  rl(H); break;
                case 0x15:  rl(L); break;
                case 0x16:  rl(_HL_); break;

                case 0x0F:  rrc(A); break;
                case 0x08:  rrc(B); break;
                case 0x09:  rrc(C); break;
                case 0x0A:  rrc(D); break;
                case 0x0B:  rrc(E); break;
                case 0x0C:  rrc(H); break;
                case 0x0D:  rrc(L); break;
                case 0x0E:  rrc(_HL_); break;

                case 0x1F:  rr(A); break;
                case 0x18:  rr(B); break;
                case 0x19:  rr(C); break;
                case 0x1A:  rr(D); break;
                case 0x1B:  rr(E); break;
                case 0x1C:  rr(H); break;
                case 0x1D:  rr(L); break;
                case 0x1E:  rr(_HL_); break;

                case 0x27:  sla(A); break;
                case 0x20:  sla(B); break;
                case 0x21:  sla(C); break;
                case 0x22:  sla(D); break;
                case 0x23:  sla(E); break;
                case 0x24:  sla(H); break;
                case 0x25:  sla(L); break;
                case 0x26:  sla(_HL_); break;

                case 0x2F:  sra(A); break;
                case 0x28:  sra(B); break;
                case 0x29:  sra(C); break;
                case 0x2A:  sra(D); break;
                case 0x2B:  sra(E); break;
                case 0x2C:  sra(H); break;
                case 0x2D:  sra(L); break;
                case 0x2E:  sra(_HL_); break;

                case 0x3F:  srl(A); break;
                case 0x38:  srl(B); break;
                case 0x39:  srl(C); break;
                case 0x3A:  srl(D); break;
                case 0x3B:  srl(E); break;
                case 0x3C:  srl(H); break;
                case 0x3D:  srl(L); break;
                case 0x3E:  srl(_HL_); break;

                default:
                    /* check for BIT op */
                    /* -x-- -xxx are relevant for the operation */
                    switch(instr & 0x47)
                    {
                        /* --xx x--- are relevant as argument */
                        case 0x47:  bit(((instr & 0x38) >> 3), A);   break;
                        case 0x40:  bit(((instr & 0x38) >> 3), B);   break;
                        case 0x41:  bit(((instr & 0x38) >> 3), C);   break;
                        case 0x42:  bit(((instr & 0x38) >> 3), D);   break;
                        case 0x43:  bit(((instr & 0x38) >> 3), E);   break;
                        case 0x44:  bit(((instr & 0x38) >> 3), H);   break;
                        case 0x45:  bit(((instr & 0x38) >> 3), L);   break;
                        case 0x46:  bit(((instr & 0x38) >> 3), _HL_);break;

                        case 0xC7:  set(((instr & 0x38) >> 3), A);   break;
                        case 0xC0:  set(((instr & 0x38) >> 3), B);   break;
                        case 0xC1:  set(((instr & 0x38) >> 3), C);   break;
                        case 0xC2:  set(((instr & 0x38) >> 3), D);   break;
                        case 0xC3:  set(((instr & 0x38) >> 3), E);   break;
                        case 0xC4:  set(((instr & 0x38) >> 3), H);   break;
                        case 0xC5:  set(((instr & 0x38) >> 3), L);   break;
                        case 0xC6:  set(((instr & 0x38) >> 3), _HL_);break;

                        case 0x87:  res(((instr & 0x38) >> 3), A);   break;
                        case 0x80:  res(((instr & 0x38) >> 3), B);   break;
                        case 0x81:  res(((instr & 0x38) >> 3), C);   break;
                        case 0x82:  res(((instr & 0x38) >> 3), D);   break;
                        case 0x83:  res(((instr & 0x38) >> 3), E);   break;
                        case 0x84:  res(((instr & 0x38) >> 3), H);   break;
                        case 0x85:  res(((instr & 0x38) >> 3), L);   break;
                        case 0x86:  res(((instr & 0x38) >> 3), _HL_);break;

                        default:
                        std::cout << "Unknown instruction 0x" << std::hex << (int)last_instr
                            << " at adr 0x" << std::hex << (int)last_adr << std::endl;
                        panic();
                            break;
                    }
                    break;
            }
            break;


        case 0xC3:  data16.r8.l = read_mem(); data16.r8.h = read_mem(); jp(data16.r16);     break;
        case 0xC2:  data16.r8.l = read_mem(); data16.r8.h = read_mem(); jp(NZ, data16.r16); break;
        case 0xCA:  data16.r8.l = read_mem(); data16.r8.h = read_mem(); jp(Z, data16.r16);  break;
        case 0xD2:  data16.r8.l = read_mem(); data16.r8.h = read_mem(); jp(NC, data16.r16); break;
        case 0xDA:  data16.r8.l = read_mem(); data16.r8.h = read_mem(); jp(CC, data16.r16); break;
        case 0xE9:  jphl(); break;

        case 0x18:  data8 = read_mem(); jr(data8);      break;
        case 0x20:  data8 = read_mem(); jr(NZ, data8);  break;
        case 0x28:  data8 = read_mem(); jr(Z, data8);   break;
        case 0x30:  data8 = read_mem(); jr(NC, data8);  break;
        case 0x38:  data8 = read_mem(); jr(CC, data8);  break;

        case 0xCD:  data16.r8.l = read_mem(); data16.r8.h = read_mem(); call(data16.r16);       break;
        case 0xC4:  data16.r8.l = read_mem(); data16.r8.h = read_mem(); call(NZ, data16.r16);   break;
        case 0xCC:  data16.r8.l = read_mem(); data16.r8.h = read_mem(); call(Z, data16.r16);    break;
        case 0xD4:  data16.r8.l = read_mem(); data16.r8.h = read_mem(); call(NC, data16.r16);   break;
        case 0xDC:  data16.r8.l = read_mem(); data16.r8.h = read_mem(); call(CC, data16.r16);   break;

        case 0xC7: rst(0x00);     break;
        case 0xCF: rst(0x08);     break;
        case 0xD7: rst(0x10);     break;
        case 0xDF: rst(0x18);     break;
        case 0xE7: rst(0x20);     break;
        case 0xEF: rst(0x28);     break;
        case 0xF7: rst(0x30);     break;
        case 0xFF: rst(0x38);     break;

        case 0xC9:  ret();      break;
        case 0xC0:  ret(NZ);    break;
        case 0xC8:  ret(Z);     break;
        case 0xD0:  ret(NC);    break;
        case 0xD8:  ret(CC);    break;

        case 0xD9:  reti(); break;

        default:
            std::cout << "Unknown instruction 0x" << std::hex << (int)last_instr
                << " at adr 0x" << std::hex << (int)last_adr << std::endl;
            panic();
                break;
    }

#ifdef BREAKPOINT
    if (last_adr == BREAKPOINT)
    {
        std::cout << "Breakpoint reached" << std::endl;
        panic();
        return;
    }
#endif
}
