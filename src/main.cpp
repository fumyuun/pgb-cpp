#include "gameboy.h"
#include <iostream>

int main( int argc, char* args[] )
{
//    gameboy_t gb(true, "01-special.gb");
//    gameboy_t gb(false, "06-ld r,r.gb");

    gameboy_t gb(true, "cpu_instrs.gb");

//	gameboy_t gb(true, "tetris.gb");
//	gameboy_t gb(true, "pokemon_blue.gb");

//	gameboy_t gb(true, "hello.gb");

	gb.run();

	return 0;
}
