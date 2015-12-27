#include "gameboy.h"
#include <iostream>
#include <boost/filesystem.hpp>

int main( int argc, char* argv[] )
{
    if(argc < 2){
        std::cerr << "Usage: " << argv[0] << " <rom>" << std::endl;
        return -1;
    }

    bool const emulate_boot_rom = boost::filesystem::exists("boot_rom.bin");
    gameboy_t gb(false, argv[1]);
    gb.run();

    return 0;
}
