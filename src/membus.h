#ifndef MEMBUS_H
#define MEMBUS_H

#include <string>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdint.h>

class membus_t
{
	private:
		uint8_t rom[0xFFFF];
		uint8_t ram[0xFFFF];
		uint8_t bootrom[0xFF];
		uint8_t *cart_mode;
		uint8_t *rom_size;
		uint8_t *ram_size;
		uint8_t mem_mode;
		uint8_t rom_bank;
		bool bootrom_enabled;
		bool panicked;
		void panic();

	public:
		membus_t();
		bool open_bootrom();
		bool open_rom(std::string filename);
		uint8_t read(const uint16_t addr);
		void write(const uint16_t addr, const uint8_t val);
		bool is_panicked();
		void enable_bootrom();
		void disable_bootrom();
		uint8_t *get_pointer(const uint16_t addr);
};

#endif
