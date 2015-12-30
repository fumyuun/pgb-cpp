#include "cpu.h"

void cpu_t::add(const reg8 src)
{
    int result = *get_reg(A) + src;
    if((result & 0xFF) == 0x00)
        *get_reg(F) |= FLAG_Z;
    else
        *get_reg(F) &= ~FLAG_Z;
    *get_reg(F) &= ~FLAG_N;
    if((*get_reg(A) & 0x08) && (src & 0x08))
        *get_reg(F) |= FLAG_H;
    if(result > 0xFF)
        *get_reg(F) |= FLAG_C;
    else
        *get_reg(F) &= ~FLAG_C;
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
    if((*get_reg(A) & 0xFF) == 0x00)
        *get_reg(F) |= FLAG_Z;
    else
        *get_reg(F) &= ~FLAG_Z;
    *get_reg(F) &= ~FLAG_N;
    *get_reg(F) &= ~FLAG_H;
}

void cpu_t::sub(const reg8 src)
{
    reg8 result = *get_reg(A) - src;
//  if(*get_reg(F) & FLAG_C)
//      result -= 0x80;
    if((result & 0xFF) == 0x00)
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
    if (*get_reg(F) & FLAG_C)
        result -= 1;

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
    if(result & 0x08)
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
    std::cout << "Halt" << std::endl;
    halted = true;
}

void cpu_t::stop()
{
    std::cout << "STOP!" << std::endl;
    panic();
}

void cpu_t::di()
{
    std::cout << "DI" << std::endl;
    IME = false;
}

void cpu_t::ei()
{
    std::cout << "EI" << std::endl;
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

void cpu_t::ldhna_byte(const uint8_t n)
{
    ldhna_word(0xFF00 + n);
}

void cpu_t::ldhan_byte(const uint8_t n)
{
    ldhan_word(0xFF00 + n);
}

void cpu_t::ldhlspn_byte(const uint8_t n)
{
    reg8 src = *get_reg(SP);
    *get_reg(HL) = membus->read(*get_reg(SP) + n);
    set_flags(false, false, ((src & 0x800) & (*get_reg(HL) & 0x800)), ((src & 0x8000) & (*get_reg(HL) & 0x8000)));
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

void cpu_t::sll(const reg8_e dest)
{
    if(*get_reg(dest) & 0x01)
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
#define C_CC    ((*get_reg(F) & FLAG_C) == FLAG_C)

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
//    std::cout << "[" << *get_reg(PC) << "]JR -> " << std::hex << (*get_reg(PC) + d) << std::endl;
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
