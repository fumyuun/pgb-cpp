#include "gameboy.h"
 
void gameboy_t::panic()
{
	std::cout << "========\nGameboy panicked!\n========\n";
	panicked = true;
}

gameboy_t::gameboy_t(bool bootrom_enabled, std::string rom_filename)
: panicked(false)
{
	if(bootrom_enabled)
		bootrom_enabled = memory.open_bootrom();
	if(!memory.open_rom(rom_filename))
		std::cout << "GameROM not found\n";

	cpu.init(&memory, bootrom_enabled);
	videodec.init(&memory);
}

#define	IS_PANICKED	(cpu.is_panicked() || memory.is_panicked() || videodec.is_panicked())
void gameboy_t::run()
{
	while( !IS_PANICKED )
	{
		cpu.run();
		videodec.run();
		if(videodec.requesting_debug())
			cpu.print();
	}
	panic();
}

bool gameboy_t::is_panicked()
{
	return panicked;
}
