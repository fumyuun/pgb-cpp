#ifndef GAMEBOY_H
#define GAMEBOY_H

#include "cpu.h"
#include "membus.h"
#include "sys/time.h"

#include "sdl_videodec.h"

class gameboy_t
{
	private:
	cpu_t cpu;
	membus_t memory;
	sdl_videodec_t videodec;
	bool panicked;
	void panic();

	public:
	gameboy_t(bool, std::string);
	void run();
	bool is_panicked();
};

#endif
