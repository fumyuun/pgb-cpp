#ifndef MEMBUS_H
#define MEMBUS_H

#include <string>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdint.h>

#define KEYMASK_UP		0x04
#define KEYMASK_LEFT	0x02
#define KEYMASK_RIGHT	0x01
#define KEYMASK_DOWN	0x08
#define KEYMASK_A		0x01
#define KEYMASK_B		0x02
#define KEYMASK_START	0x08
#define KEYMASK_SELECT	0x04

typedef enum {
	KEY_UP,
	KEY_LEFT,
	KEY_RIGHT,
	KEY_DOWN,
	KEY_A,
	KEY_B,
	KEY_START,
	KEY_SELECT
} jskey_t;

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
		bool key_states[8];
		bool keypad_selected;

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

		void set_keydown(jskey_t key);
		void set_keyup(jskey_t key);
		void keypad_select_buttons();
		void keypad_select_direction();
		void keypad_update();
};

#endif
