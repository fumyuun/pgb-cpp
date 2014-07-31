#include "gameboy.h"

#include <boost/thread.hpp>
 
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

void gameboy_t::run()
{
    typedef boost::thread thread;

    struct cpu_runner_t {
        cpu_runner_t(cpu_t& cpu_, membus_t& memory_)
        : cpu(cpu_)
        , memory(memory_)
        {}

        cpu_t& cpu;
        membus_t& memory;

        void operator()(){
            while(!(cpu.is_panicked() || memory.is_panicked())){
                cpu.run();
            }
            throw std::runtime_error("CPU panicked");
        }
    };

    cpu_runner_t cpu_runner(cpu, memory);
    thread cpu_thread(cpu_runner);

    while( !videodec.is_panicked() )
    {
        videodec.run();
        if(videodec.requesting_debug()){
            cpu.print();
        }
    }
    throw std::runtime_error("Videodec panicked");

    cpu_thread.join();
}

bool gameboy_t::is_panicked()
{
	return panicked;
}
