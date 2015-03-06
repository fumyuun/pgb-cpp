#include "cpu.h"

#include "common.h"

#include <boost/thread.hpp>
#include <boost/chrono/system_clocks.hpp>

std::string binstring(const unsigned char byte);
std::string binstring(const unsigned short bytes);
std::string reg8_e_tostring(const reg8_e r);
std::string reg16_e_tostring(const reg16_e r);
std::string cond_e_tostring(const cond_e cc);
std::string alu_e_tostring(const alu_e alu);
std::string rot_e_tostring(const rot_e rot);

void cpu_t::init(membus_t *membus_, bool bootrom_enabled)
{
    last_instr.instr = 0x00;
    last_instr.data8 = 0x00;
    last_instr.data16.r16 = 0x00;
    last_instr.adr = 0x00;
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
    IF = membus->get_pointer(0xFF0F);
}

void cpu_t::run()
{
    if(!panicked)
    {
        using namespace boost::chrono;
        typedef high_resolution_clock clock;
        time_point<clock> const start = clock::now();
        if(!halted && cycles_left == 0)
        {
            id_execute();
        }
        if(cycles_left > 0) --cycles_left;
//      inc_counters();
        check_interrupts();

        nanoseconds nano_sleepy(microseconds(40) - (clock::now() - start));
        //boost::this_thread::sleep_for(nano_sleepy);
    }
}

int8_t cpu_t::read_mem()
{
    //last_instr.adr = *get_reg(PC);
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
            std::cout << "LCDSTAT" << std::endl;
            *IF &= ~FLAG_I_LCDSTAT;
            call(0x48);
        }

        if(flags & FLAG_I_TIMER)
        {
            std::cout << "TIMER" << std::endl;
            *IF &= ~FLAG_I_TIMER;
            call(0x50);
        }

        if(flags & FLAG_I_SERIAL)
        {
            std::cout << "SERIAL" << std::endl;
            *IF &= ~FLAG_I_SERIAL;
            call(0x58);
        }

        if(flags & FLAG_I_JOYPAD)
        {
            std::cout << "JOYPAD" << std::endl;
            *IF &= ~FLAG_I_JOYPAD;
            call(0x60);
        }
    }
}

void cpu_t::inc_counters()
{
    if(membus->read(0xFF44) == 0x90)
    {
        //std::cout << "VBLANK" << std::endl;
        *IF |= FLAG_I_VBLANK;
    }
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
        while(last_instr.instr != 0x00 && !panicked);
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
    std::cout << "Last instruction 0x" << std::hex << (int)last_instr.instr;
    #if DEBUG_OUTPUT > 0
    std::cout << "(";
    cpu_debug_print(last_instr.adr, last_instr.instr, last_instr.data8, last_instr.data16, std::cout);
    std::cout << ")";
    #endif
    std::cout << " at adr 0x" << std::hex << (int)last_instr.adr << "\n";
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
