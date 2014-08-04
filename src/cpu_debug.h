#ifndef CPU_DEBUG_H
#define CPU_DEBUG_H
#include <string>
#include <vector>
#include <algorithm>

#include "cpu.h"

void cpu_debug_print(reg8 pc, reg8 instr, reg8 data8, reg16_2x8 data16, std::ostream &out);

#endif
