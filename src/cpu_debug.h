#ifndef CPU_DEBUG_H
#define CPU_DEBUG_H
#include <string>
#include <vector>
#include <algorithm>

#include "cpu.h"


reg8_e get_r(const reg8 r);
reg16_e get_rp(const reg8 r);
reg16_e get_rp2(const reg8 r);
cond_e get_cc(const reg8 r);
alu_e get_alu(const reg8 r);
rot_e get_rot(const reg8 r);
std::string reg8_e_tostring(const reg8_e r);
std::string reg16_e_tostring(const reg16_e r);
std::string cond_e_tostring(const cond_e cc);
std::string alu_e_tostring(const alu_e alu);
std::string rot_e_tostring(const rot_e rot);
reg8 id_execute_cb(reg8 pc, reg8 instr, std::ostream &out);
reg8 cpu_debug_print(reg8 pc, reg8 instr, reg8 data8, reg16_2x8 data16, std::ostream &out);

#endif
