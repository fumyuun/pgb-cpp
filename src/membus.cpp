#include "membus.h"
#include "common.h"

#include <boost/chrono/system_clocks.hpp>

#include <cmath>
#include <cctype>
#include <iostream>
#include <iomanip>

membus_t::membus_t()
    : bootrom_enabled(false), panicked(false)
{
    int i;
    memset(rom, 0x00, 0xFFFF);	// Zero memory, not completely correct...
    memset(ram, 0x00, 0xFFFF);
    cart_mode = rom + 0x0147;
    rom_size = rom + 0x0148;
    ram_size = rom + 0x0149;
    mem_mode = 0x00;
    rom_bank = 0x00;
    for (i = 0; i < 8; ++i)
    {
        key_states[i] = false;
    }
}

bool membus_t::open_bootrom()
{
    std::ifstream rom_in("boot_rom.bin", std::ios::in | std::ios::binary);
    if(!rom_in.is_open())
        return false;

    rom_in.read((char*)bootrom, 0xFF);
    rom_in.close();
    bootrom_enabled = true;

    std::cout << "Loaded bootrom\n";
    return true;
}

bool membus_t::open_rom(std::string filename)
{
    std::ifstream::pos_type size;
    std::ifstream rom_in(filename.c_str(),
                         std::ios::in | std::ios::binary );
    if(!rom_in.good())
        return false;

    rom_in.seekg(0, std::ios::end);
    size = rom_in.tellg();
    if(size > 0x8000)
    {
        size = 0x8000;
        std::cout << "Warning, rom bigger than 0x8000" << std::endl;
    }
    rom_in.seekg(0, std::ios::beg);
    if(!rom_in.read((char*)rom, size))
        return false;
    rom_in.close();

    char name[0x11];
    strncpy(name, (char*)(rom + 0x134), 0x10);
    std::cout << "Loaded rom " << name << "(size: " << (int)size << ", cart mode: " << (int)*cart_mode;
    std::cout << ", rom size: " << (int)*rom_size << ", ram size: " << (int)*ram_size << ")" << std::endl;



    return true;
}

//! Maps @param value to a range of [0, 1]
//! Example to map 0-255 to 0-1: normalize(0, 255, 128)
//! Example to map 1-10 to 0-1: normalize(1, 10, 3)
template <typename T>
T normalize(T const min, T const max, T const value){
    return (value-min)/(max-min);
}

//! Maps @param value to [@param min , @param max ]
//! Example to map 0-1 to 0-255: normalize(0, 255, 0.5)
//! Example to map 1-10 to 0-1: normalize(1, 10, 0.4)
template <typename T>
T denormalize(T const min, T const max, T const value){
    return (value * (max-min) + min);
}

uint8_t membus_t::read(const uint16_t addr)
{
    if(addr == 0xFF00){
        std::cout << "P1 (Joypad) read: " << std::hex << (int)rom[0xFF00] << std::endl;
    }
    if(addr == 0xFF01){
        std::cout << "SB (Serial Bus) read unhandled" << std::endl;
    }
    if(addr == 0xFF02){
        std::cout << "SC (Serial Control) read unhandled" << std::endl;
    }
    if(addr == 0xFF04){
        std::cout << "DIV read unhandled" << std::endl;
    }
    if(addr == 0xFF05){
        std::cout << "TIMA read unhandled" << std::endl;
    }
    if(addr == 0xFF06){
        std::cout << "TMA read unhandled " << std::endl;
    }
    if(addr == 0xFF07){
        std::cout << "TAC read unhandled " << std::endl;
    }
    if(addr == 0xFF24){
        std::cout << "Sound channel control read unhandled " << std::endl;
    }
    if(addr == 0xFF25){
        std::cout << "Sound channel selection read unhandled " << std::endl;
    }
    if(addr == 0xFF26){
        std::cout << "Sound hardware control read unhandled " << std::endl;
    }
    if(addr == 0xFF41){
        std::cout << "STAT read unhandled" << std::endl;
    }
    if(addr == 0xFF4A){
        std::cout << "Read from WY register, unhandled" << std::endl;
    }
    if(addr == 0xFF4B){
        std::cout << "Read from WX register, unhandled" << std::endl;
    }
    // Read the LY register, we calculate what would be in there (goes from 0x00 to 0x99 at 57Hz)
    if(addr == 0xFF44){
        static const bool increment_per_read = false;
        uint8_t result = 0xA0;
        if(!increment_per_read){
            using namespace boost::chrono;
            typedef high_resolution_clock clock;
            nanoseconds const step = (nanoseconds(seconds(1))/57);
            assert(step.count() > 0);
            nanoseconds const now_duration = clock::now().time_since_epoch();

            nanoseconds const part = now_duration % step;
            double const normalized = normalize(0., double(step.count()), double(part.count()));
            double const denormalized = denormalize(0., 152., normalized);
            result = std::floor(denormalized);
            //std::cout << now_duration.count() << " " << part.count() << " -> " << int(result) << std::endl;
        } else {
            result = rom[0xFF44] = (rom[0xFF44] + 1) % 0xA0;
            //std::cout << int(result) << std::endl;
        }

        if(result == 0x90){
            // If LY == 0x90, or IF
            *get_pointer(0xFFFE) |= FLAG_I_VBLANK;
        }

        assert(result <= 0x99);
        return result;
    }

    if(bootrom_enabled && addr < 0x100)
    {
        return bootrom[addr];
    }
    /*if(addr >= 0x4000 && *cart_mode == 0x01)
    {
        if(rom_bank == 0x00)
            return rom[0x2000 + addr];
        return rom[rom_bank * 0x2000 + addr];
    }*/
    return rom[addr];
}

void membus_t::write(const uint16_t addr, const uint8_t val)
{
    if(addr == 0xFF00){
        if((val & 0x10) == 0x00)
        {
            std::cout << "P1 (Joypad) selected direction keys" << std::endl;
            keypad_select_direction();
        }
        if((val & 0x20) == 0x00)
        {
            std::cout << "P1 (Joypad) selected button keys" << std::endl;
            keypad_select_buttons();
        }
    }
    if(addr == 0xFF06){
        std::cout << "TMA write unhandled " << std::hex << (int)val << std::endl;
    }
    if(addr == 0xFF07){
        std::cout << "TAC write unhandled " << std::hex << (int)val << std::endl;
    }
    if(addr == 0xFF24){
        std::cout << "Sound channel control write unhandled " << std::hex << (int)val << std::endl;
    }
    if(addr == 0xFF25){
        std::cout << "Sound channel selection write unhandled " << std::hex << (int)val << std::endl;
    }
    if(addr == 0xFF26){
        std::cout << "Sound hardware control write unhandled " << std::hex << (int)val << std::endl;
    }
    if(addr == 0xFF40){
        std::cout << "LCDC write " << std::hex << (unsigned int)val << std::endl;
    }
    if(addr == 0xFF41){
        std::cout << "STAT write " << std::hex << (unsigned int)val << std::endl;
    }
    if(addr == 0xFF44){
        std::cout << "Written to LY register, unhandled" << std::endl;
    }
    if(addr == 0xFF46){
        std::cout << "Written to DMA register, unhandled" << std::endl;
    }
    if(addr == 0xFF4A){
        std::cout << "Written to WY register, unhandled" << std::endl;
    }
    if(addr == 0xFF4B){
        std::cout << "Written to WX register, unhandled" << std::endl;
    }
    if(addr == 0xFF0F){
        std::cout << "IF write: " << std::hex << (unsigned int)val << std::endl;
    }
    if(addr == 0xFFFF){
        std::cout << "IE write: " << std::hex << (unsigned int)val << std::endl;
    }
    if((addr == 0xFF02) && (val & 0x80) && std::isprint(rom[0xFF01])){
        std::cout << (char)rom[0xFF01] << std::flush;
    }
    if(addr >= 0x6000 && addr < 0x8000 && *cart_mode == 0x01)
    {
        std::cout << "Memory mode write " << val << std::endl;
        //mem_mode = val & 0x01;
    }

    if(addr >= 0x2000 && addr < 0x8000 && *cart_mode == 0x01)
    {
        std::cout << "Memory mode write " << val << std::endl;
        //mem_mode = val & 0x01;
    }

    if(bootrom_enabled && addr == 0xFF50 && val == 0x01){
        std::cout << "Disabling boot ROM." << std::endl;
        bootrom_enabled = false;
    }
    //std::cout << "Write [" << std::hex << addr << "]=" << std::hex << (int)val << std::endl;
    rom[addr] = val;
}

uint8_t *membus_t::get_pointer(const uint16_t addr)
{
    return &rom[addr];
}

void membus_t::panic()
{
    panicked = true;
}

bool membus_t::is_panicked()
{
    return panicked;
}

void membus_t::enable_bootrom()
{
    std::cout << "Enabling boot ROM.\n";
    bootrom_enabled = true;
}

void membus_t::disable_bootrom()
{
    std::cout << "Disabling boot ROM.\n";
    bootrom_enabled = false;
}

void membus_t::set_keydown(jskey_t key)
{
    key_states[key] = true;
    keypad_update();
}

void membus_t::set_keyup(jskey_t key)
{
    key_states[key] = false;
    keypad_update();
}

void membus_t::keypad_select_buttons()
{
    keypad_selected = false;
    keypad_update();
}

void membus_t::keypad_select_direction()
{
    keypad_selected = true;
    keypad_update();
}

void membus_t::keypad_update()
{
    int i;
    uint8_t mask = rom[0xFF00] | 0x0F;
    if(keypad_selected)
    {
        if(key_states[KEY_UP])    mask &= ~KEYMASK_UP;
        if(key_states[KEY_LEFT])  mask &= ~KEYMASK_LEFT;
        if(key_states[KEY_RIGHT]) mask &= ~KEYMASK_RIGHT;
        if(key_states[KEY_DOWN])  mask &= ~KEYMASK_DOWN;
    } else {
        if(key_states[KEY_A])      mask &= ~KEYMASK_A;
        if(key_states[KEY_B])      mask &= ~KEYMASK_B;
        if(key_states[KEY_START])  mask &= ~KEYMASK_START;
        if(key_states[KEY_SELECT]) mask &= ~KEYMASK_SELECT;
    }

    rom[0xFF00] = mask;
    std::cout << "Keypad selected: " << keypad_selected << std::endl;
    std::cout << "Key states: ";
    for (i = 0; i < 8; ++i)
    {
        std::cout << key_states[i] << ", ";
    }
    std::cout << std::endl << "Keystate: " << std::hex << (int)rom[0xFF00] << std::endl;
}