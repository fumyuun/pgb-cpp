#ifndef CPU_DEBUG_H
#define CPU_DEBUG_H
#include <string>
#include <vector>
#include <algorithm>

#include "cpu.h"
#include "membus.h"

void cpu_debug_print(reg8 pc, reg8 instr, reg8 data8, reg16_2x8 data16, std::ostream &out);
void cpu_debug_memdump(uint8_t *base, unsigned int addr, unsigned int lines);

#endif
