#include "cpu.h"

#define DEBUG_OUTPUT 0

#define FLAG_Z	0x80
#define FLAG_N	0x40
#define FLAG_H	0x20
#define FLAG_C	0x10

#define FLAG_I_JOYPAD	0x10
#define FLAG_I_SERIAL	0x08
#define FLAG_I_TIMER	0x04
#define FLAG_I_LCDSTAT	0x02
#define FLAG_I_VBLANK	0x01

std::string binstring(const unsigned char byte);
std::string binstring(const unsigned short bytes);
std::string reg8_e_tostring(const reg8_e r);
std::string reg16_e_tostring(const reg16_e r);
std::string cond_e_tostring(const cond_e cc);
std::string alu_e_tostring(const alu_e alu);
std::string rot_e_tostring(const rot_e rot);

void cpu_t::init(membus_t *membus_, bool bootrom_enabled)
{
	last_instr = 0x00;
	last_adr = 0x00;
	panicked = false;
	booted = false;
	halted = false;
	IME = false;
	membus = membus_;
	cycles_left = 0;
	for(int i = 0; i < 6; ++i)	// To make valgrind happy, shouldn't be here.
		registers[i].r16 = 0x00;
	*get_reg(PC) = (bootrom_enabled ? 0x0000 : 0x0100);
	IE = membus->get_pointer(0xFFFF);
	IF = membus->get_pointer(0xFFFE);
}

void cpu_t::run()
{
	if(!panicked)
	{
		if(!halted && cycles_left == 0)
		{
			id_execute();
		}
		if(cycles_left > 0) --cycles_left;
//		inc_counters();
		check_interrupts();
	}
}

int8_t cpu_t::read_mem()
{
	last_adr = *get_reg(PC);
	int8_t c = membus->read((*get_reg(PC))++);
	return c;
}

void cpu_t::check_interrupts()
{
	if(!IME)
		return;
//	std::cout << "IME: " << IME << " IE: " << std::hex << (unsigned int)*IE << " IF: " << std::hex << (unsigned int)*IF << std::endl;

	reg8 flags = *IE & *IF;
	if(flags)
	{
		IME = false;
		halted = false;
		if(flags & FLAG_I_VBLANK)
		{
			std::cout << "VBLANK" << std::endl;
			*IF &= ~FLAG_I_VBLANK;
			call(0x40);
		}

		if(flags & FLAG_I_LCDSTAT)
		{
			*IF &= ~FLAG_I_LCDSTAT;
			call(0x48);
		}

		if(flags & FLAG_I_TIMER)
		{
			*IF &= ~FLAG_I_TIMER;
			call(0x50);
		}

		if(flags & FLAG_I_SERIAL)
		{
			*IF &= ~FLAG_I_SERIAL;
			call(0x58);
		}

		if(flags & FLAG_I_JOYPAD)
		{
			*IF &= ~FLAG_I_JOYPAD;
			call(0x60);
		}
	}
}

void cpu_t::inc_counters()
{
	reg8 *ly = membus->get_pointer(0xFF44);
	if(*ly == 0x90)
	{
		//std::cout << "VBLANK" << std::endl;
		*IF |= FLAG_I_VBLANK;
	}
	*ly += 1;

	// std::cout << (std::hex) << (unsigned int) *ly << std::endl;
/*	if(*ly > 0x99)
	{
	//	*ly = 0;	// sad hack is sad :(
		*ly = 0x90;
	}*/
}

reg8 *cpu_t::get_reg(reg8_e reg)
{
	switch(reg)
	{
		case A:		return &(registers[AF].r8.h);
		case F:		return &(registers[AF].r8.l);
		case B:		return &(registers[BC].r8.h);
		case C:		return &(registers[BC].r8.l);
		case D:		return &(registers[DE].r8.h);
		case E:		return &(registers[DE].r8.l);
		case H:		return &(registers[HL].r8.h);
		case L:		return &(registers[HL].r8.l);
		default:	return 0x00;	// Crash!
	}
}

reg16 *cpu_t::get_reg(reg16_e reg)
{
	switch(reg)
	{
		case AF:	return &(registers[AF].r16);
		case BC:	return &(registers[BC].r16);
		case DE:	return &(registers[DE].r16);
		case HL:	return &(registers[HL].r16);
		case SP:	return &(registers[SP].r16);
		case PC:
		default:	return &(registers[PC].r16);
	}
}

reg8_e cpu_t::get_r(const reg8 r)
{
	switch(r)
	{
		case 0x00:	return B;
		case 0x01:	return C;
		case 0x02:	return D;
		case 0x03:	return E;
		case 0x04:	return H;
		case 0x05:	return L;
		case 0x06:	return _HL_;
		case 0x07:
		default:	return A;
	}
}
reg16_e cpu_t::get_rp(const reg8 r)
{
	switch(r)
	{
		case 0x00:	return BC;
		case 0x01:	return DE;
		case 0x02:	return HL;
		case 0x03:
		default:	return SP;
	}
}

reg16_e cpu_t::get_rp2(const reg8 r)
{
	switch(r)
	{
		case 0x00:	return BC;
		case 0x01:	return DE;
		case 0x02:	return HL;
		case 0x03:
		default:	return AF;
	}
}

cond_e cpu_t::get_cc(const reg8 r)
{
	switch(r)
	{
		case 0x00:	return NZ;
		case 0x01:	return Z;
		case 0x02:	return NC;
		case 0x03:	return CC;
		case 0x04:	return PO;
		case 0x05:	return PE;
		case 0x06:	return P;
		case 0x07:
		default:	return M;
	}
}

alu_e cpu_t::get_alu(const reg8 r)
{
	switch(r)
	{
		case 0x00:	return ADD_A;
		case 0x01:	return ADC_A;
		case 0x02:	return SUB;
		case 0x03:	return SBC_A;
		case 0x04:	return AND;
		case 0x05:	return XOR;
		case 0x06:	return OR;
		case 0x07:
		default:	return CP;
	}
}

rot_e cpu_t::get_rot(const reg8 r)
{
	switch(r)
	{
		case 0x00:	return RLC;
		case 0x01:	return RRC;
		case 0x02:	return RL;
		case 0x03:	return RR;
		case 0x04:	return SLA;
		case 0x05:	return SRA;
		case 0x06:	return SLL;
		case 0x07:
		default:	return SRL;
	}
}

#define GET_X(x)	((x & 0xC0) >> 6)
#define GET_Y(x)	((x & 0x38) >> 3)
#define GET_Z(x)	(x & 0x07)
#define GET_P(x)	((x & 0x30) >> 4)
#define GET_Q(x)	((x & 0x08) >> 3)
void cpu_t::id_execute()
{
	last_adr = *get_reg(PC);
	reg8 instr = read_mem();
	last_instr = instr;

	if(!booted && last_adr > 0xFF)
	{
		booted = true;
		membus->disable_bootrom();
	}

#if DEBUG_OUTPUT > 0
	if(!booted)
		std::cout << "[BOOT]";

	std::cout << "PC: " << std::hex << (int)last_adr << " INSTR: "
		<< (int)last_instr << " ";
#endif
#if DEBUG_OUTPUT == 1
	std::cout << "\n";
#endif

	reg8 x = GET_X(instr);
	reg8 y = GET_Y(instr);
	reg8 z = GET_Z(instr);
	reg8 p = GET_P(instr);
	reg8 q = GET_Q(instr);
	reg16_e rp = AF;
	reg8_e r = A, r2 = A;
	reg16_2x8 data16; data16.r16 = 0x00;
	reg8 data8 = 0x00;
	cond_e cc = NZ;
	alu_e alu = ADD_A;

	switch(x)
	{
	case 0x00:
		switch(z)
		{
		case 0x00:
			switch(y)
			{
			case 0x00:
					#if DEBUG_OUTPUT == 2
						std::cout << "NOP\n";
					#endif
					nop(); 					// NOP
						cycle(4);
						return;
			case 0x01:
				data16.r8.l = read_mem();
				data16.r8.h = read_mem();
				#if DEBUG_OUTPUT == 2
					std::cout << "LD ("
						<< std::hex << (int)data16.r16 << "), SP\n";
				#endif
				ldhnnsp(data16.r16);			// LD (nn),SP
				cycle(20);
				return;
			case 0x02:
						#if DEBUG_OUTPUT == 2
							std::cout << "STOP\n";
						#endif
						stop(); 				// STOP ??
						cycle(4);
						return;
			case 0x03:	data8 = read_mem();
						#if DEBUG_OUTPUT == 2
							std::cout << "JR "
								<< std::hex << (int)data8 << "\n";
						#endif
						jr(data8);				// JR r
						cycle(8);

						return;
			case 0x04:
			case 0x05:
			case 0x06:
			case 0x07:
						cc = get_cc(y - 4);
						data8 = read_mem();
						#if DEBUG_OUTPUT == 2
							std::cout << "JR " << cond_e_tostring(cc)
								<< ", " << std::hex << (int)data8 << "\n";
						#endif
						jr(cc, data8);	// JR c,d
						cycle(8);
						return;
			default:	break;
			}
			break;

		case 0x01:
			switch(q)
			{
			case 0x00:
				rp = get_rp(p);
				data16.r8.l = read_mem();
				data16.r8.h = read_mem();
				#if DEBUG_OUTPUT == 2
					std::cout << "LD " << reg16_e_tostring(rp)
						<< ", " << std::hex << (int)data16.r16 << "\n";
				#endif
				ld(rp, data16);		// LD r,nn
				cycle(12);
				return;
			case 0x01:
				rp = get_rp(p);
				#if DEBUG_OUTPUT == 2
					std::cout << "ADD HL, " << reg16_e_tostring(rp) << "\n";
				#endif
				addhl(get_rp(p));			// ADD HL,rr
				cycle(8);
				return;
			default:	break;
			}
			break;

		case 0x02:
			switch((q << 2) | p)
			{
			case 0x00:						// LD (BC),A
				#if DEBUG_OUTPUT == 2
					std::cout << "LD (BC), A\n";
				#endif
				ldbca();
				cycle(8);
				return;
			case 0x01:						// LD (DE),A
				#if DEBUG_OUTPUT == 2
					std::cout << "LD (DE), A\n";
				#endif
				lddea();
				cycle(8);
				return;
			case 0x02:						// LDI (HL),A
				#if DEBUG_OUTPUT == 2
					std::cout << "LDI (HL), A\n";
				#endif
				ldihla();
				cycle(8);
				return;
			case 0x03:						// LDD (HL),A
				#if DEBUG_OUTPUT == 2
					std::cout << "LDD (HL), A\n";
				#endif
				lddhla();
				cycle(8);
				return;
			case 0x04:						// LD A,(BC)
				#if DEBUG_OUTPUT == 2
					std::cout << "LD A, (BC)\n";
				#endif
				ldabc();
				cycle(8);
				return;
			case 0x05:						// LD A,(DE)
				#if DEBUG_OUTPUT == 2
					std::cout << "LD A, (DE)\n";
				#endif
				ldade();
				cycle(8);
				return;
			case 0x06:						// LDI A,(HL)
				#if DEBUG_OUTPUT == 2
					std::cout << "LDI A, (HL)\n";
				#endif
				ldiahl();
				cycle(8);
				return;
			case 0x07:						// LDD A,(HL)
				#if DEBUG_OUTPUT == 2
					std::cout << "LDD A, (HL)\n";
				#endif
				lddahl();
				cycle(8);
				return;
			default:
				break;
			}
			break;

		case 0x03:
			rp = get_rp(p);
			if(q == 0x00)
			{
				#if DEBUG_OUTPUT == 2
					std::cout << "INC " << reg16_e_tostring(rp)<< "\n";
				#endif
				inc(rp);	// INC rr
			}
			else
			{
				#if DEBUG_OUTPUT == 2
					std::cout << "DEC " << reg16_e_tostring(rp)<< "\n";
				#endif
				dec(rp);	// DEC rr
			}
			cycle(8);
			return;

		case 0x04:
			r = get_r(y);
			#if DEBUG_OUTPUT == 2
				std::cout << "INC " << reg8_e_tostring(r) << "\n";
			#endif
			inc(r);						// INC r
			if(r == _HL_)	cycle(12);
			else			cycle(4);
			return;

		case 0x05:
			r = get_r(y);
			#if DEBUG_OUTPUT == 2
				std::cout << "DEC " << reg8_e_tostring(r) << "\n";
			#endif
			dec(r);						// DEC r
			if(r == _HL_)	cycle(12);
			else			cycle(4);
			return;

		case 0x06:
			r = get_r(y);
			data8 = read_mem();
			#if DEBUG_OUTPUT == 2
				std::cout << "LD " << reg8_e_tostring(r)
					<< std::hex << (int)data8 << "\n";
			#endif
			ld(r, data8);				// LD r,n
			cycle(8);
			return;

		case 0x07:
			switch(y)
			{
			case 0x00:
				#if DEBUG_OUTPUT == 2
					std::cout << "RLCA\n";
				#endif
				rlca();	cycle(4);	return; // RLCA
			case 0x01:
				#if DEBUG_OUTPUT == 2
					std::cout << "RRCA\n";
				#endif
				rrca();	cycle(4);	return; // RRCA
			case 0x02:
				#if DEBUG_OUTPUT == 2
					std::cout << "RLA\n";
				#endif
				rla();	cycle(4);	return; // RLA
			case 0x03:
				#if DEBUG_OUTPUT == 2
					std::cout << "RRA\n";
				#endif
				rra();	cycle(4);	return; // RRA
			case 0x04:
				#if DEBUG_OUTPUT == 2
					std::cout << "DAA\n";
				#endif
				daa();	cycle(4);	return;	// DAA
			case 0x05:
				#if DEBUG_OUTPUT == 2
					std::cout << "CPL\n";
				#endif
				cpl();	cycle(4);	return;	// CPL
			case 0x06:
				#if DEBUG_OUTPUT == 2
					std::cout << "SCF\n";
				#endif
				scf();	cycle(4);	return;	// SCF
			case 0x07:
				#if DEBUG_OUTPUT == 2
					std::cout << "CCF\n";
				#endif
				ccf();	cycle(4);	return;	// CCF
			default:   break;
			}
			break;

		default:	break;
		}
	case 0x01:
		if((z == 0x06) && (y == 0x06))
		{
			panic();						// EXCEPTION
			return;
		}
		r = get_r(y);
		r2 = get_r(z);
		#if DEBUG_OUTPUT == 2
			std::cout << "LD " << reg8_e_tostring(r) << ", "
				<< reg8_e_tostring(r2) << "\n";
		#endif
		ld(r, r2);							// LD r,r
		if(get_r(y) == _HL_ || get_r(z) == _HL_)
			cycle(8);
		else
			cycle(4);
		return;

	case 0x02:
		alu = get_alu(y);
		r = get_r(z);
		#if DEBUG_OUTPUT == 2
			std::cout << alu_e_tostring(alu) << " A, "
				<< reg8_e_tostring(r) << "\n";
		#endif
		alu_exec(alu, r);				// ALU operations on A,r
		if(r == _HL_)
			cycle(8);
		else
			cycle(4);
		return;

	case 0x03:
		switch(z)
		{
		case 0x00:
			cc = get_cc(y);
			if(cc == PO)
			{
				data8 = read_mem();
				#if DEBUG_OUTPUT == 2
					std::cout << "LDH (" << std::hex << (int)data8 << "), A\n";
				#endif
				ldhna_byte(data8);		// LDH (byte),A
				cycle(12);
				return;
			}
			if(cc == PE)
			{
				data8 = read_mem();
				#if DEBUG_OUTPUT == 2
					std::cout << "ADD SP, " << std::hex << (int)data8 << "\n";
				#endif
				addsp(data8);			// ADD SP,offset
				cycle(16);
				return;
			}
			if(cc == P)
			{
				data8 = read_mem();
				#if DEBUG_OUTPUT == 2
					std::cout << "LDH A, (" << std::hex << (int)data8 << ")\n";
				#endif
				ldhan_byte(data8);		// LDH A,(byte)
				cycle(12);
				return;
			}
			if(cc == M)
			{
				data8 = read_mem();
				#if DEBUG_OUTPUT == 2
					std::cout << "LDHL SP, " << std::hex << (int)data8 << "\n";
				#endif
				ldhlspn_byte(data8);	// LDHL SP,nn
				cycle(12);
				return;
			}
			#if DEBUG_OUTPUT == 2
				std::cout << "RET " << cond_e_tostring(cc) << "\n";
			#endif
			ret(get_cc(y));					// RET cc
			cycle(8);
			return;
		case 0x01:
			if(q == 0x00)
			{
				rp = get_rp2(p);
				#if DEBUG_OUTPUT == 2
					std::cout << "POP " << reg16_e_tostring(rp) << "\n";
				#endif
				pop(rp);					// POP rr
				cycle(12);
				return;
			}
			switch(p)
			{
			case 0x00:
				#if DEBUG_OUTPUT == 2
					std::cout << "RET\n";
				#endif
				ret();		cycle(8);	return;	// RET
			case 0x01:
				#if DEBUG_OUTPUT == 2
					std::cout << "RETI\n";
				#endif
				 reti();		cycle(8);	return;	// RETI
			case 0x02:
				#if DEBUG_OUTPUT == 2
					std::cout << "JP HL\n";
				#endif
				 jphl();		cycle(4);	return;	// JP HL
			case 0x03:
				#if DEBUG_OUTPUT == 2
					std::cout << "LD SP, HL\n";
				#endif
				 ldsphl();	cycle(12);	return;	// LD SP,HL
			}
			break;

		case 0x02:
			cc = get_cc(y);
			if(cc == PO)
			{
				#if DEBUG_OUTPUT == 2
					std::cout << "LD (C), A\n";
				#endif
				ld(_C_, A);					// LD (C),A
				cycle(8);
				return;
			}
			if(cc == PE)
			{
				data16.r8.l = read_mem();
				data16.r8.h = read_mem();
				#if DEBUG_OUTPUT == 2
					std::cout << "LD ("
						<< std::hex << (int)data16.r16 << "), A\n";
				#endif
				ldhna_word(data16.r16);		// LD (nn),A
				cycle(16);
				return;
			}
			if(cc == M)
			{
				data16.r8.l = read_mem();
				data16.r8.h = read_mem();
				#if DEBUG_OUTPUT == 2
					std::cout << "LD A, ("
						<< std::hex << (int)data16.r16 << ")\n";
				#endif
				ldhan_word(data16.r16);		// LD A,(nn)
				cycle(16);
				return;
			}
			if(cc == P)
			{
				panic();
				return;
			}
			data16.r8.l = read_mem();
			data16.r8.h = read_mem();
			#if DEBUG_OUTPUT == 2
				std::cout << "JP " << cond_e_tostring(cc) << ", "
					<< std::hex << (int)data16.r16 << "\n";
			#endif
			jp(cc, data16.r16);				// JP CC,nn
			cycle(12);
			return;
		case 0x03:
			switch(y)
			{
			case 0x00:
				data16.r8.l = read_mem();
				data16.r8.h = read_mem();
				#if DEBUG_OUTPUT == 2
					std::cout << "JP " << std::hex << (int)data16.r16 << "\n";
				#endif
				jp(data16.r16);				// JP nn
				cycle(12);
				return;

			case 0x01:
				id_execute_cb();			// CB-prefixed operations
				return;
			case 0x02:						// z80-OUT, removed
			case 0x03:						// z80-IN,	removed
			case 0x04:						// z80-EX,	removed
			case 0x05:						// z80-EX,	removed
				panic();
				return;
			case 0x06:
				#if DEBUG_OUTPUT == 2
					std::cout << "DI\n";
				#endif
				di();						// DI
				cycle(4);
				return;
			case 0x07:
				#if DEBUG_OUTPUT == 2
					std::cout << "EI\n";
				#endif
				ei();						// EI
				cycle(4);
				return;
			default:	break;
			}
			break;

		case 0x04:
			cc = get_cc(y);
			data16.r8.l = read_mem();
			data16.r8.h = read_mem();
			#if DEBUG_OUTPUT == 2
				std::cout << "CALL " << cond_e_tostring(cc) << ", "
					<< std::hex << (int)data16.r16 << "\n";
			#endif
			call(cc, data16.r16);			// CALL CC,NN
			cycle(12);
			return;

		case 0x05:
			if(q == 0x00)
			{
				rp = get_rp2(p);
				#if DEBUG_OUTPUT == 2
					std::cout << "PUSH " << reg16_e_tostring(rp) << "\n";
				#endif
				push(rp);					// PUSH rr
				cycle(16);
				return;
			}
			switch(p)
			{
			case 0x00:
				data16.r8.l = read_mem();
				data16.r8.h = read_mem();
				#if DEBUG_OUTPUT == 2
					std::cout << "CALL " << std::hex << (int)data16.r16 << "\n";
				#endif
				call(data16.r16);			// CALL nn
				cycle(12);
				return;
			case 0x01:						// z80-DD-prefix, removed
			case 0x02:						// z80-ED-prefix, removed
			case 0x03: panic();	return;		// z80-FD-prefix, removed
			}
			break;

		case 0x06:
			alu = get_alu(y);
			data8 = read_mem();
			#if DEBUG_OUTPUT == 2
				std::cout << alu_e_tostring(alu) << " A, "
					<< std::hex << (int)data8 << "\n";
			#endif
			alu_exec(get_alu(y), data8);	// ALU operations on A,n
			cycle(8);
			return;

		case 0x07:
			#if DEBUG_OUTPUT == 2
				std::cout << "RST" << std::hex << (int)(8*y) << "\n";
			#endif
			rst(8*y);						// RST n
			cycle(32);
			return;

		default:	break;
		}
		break;
	}

	std::cout << "Unknown instruction 0x" << std::hex << (int)instr
		<< " at adr 0x" << std::hex << (int)(*get_reg(PC))-1 << "\n"
		<< "x=" << (int)x << " y=" << (int)y << " z=" << (int)z
		<< " p=" << (int)p << " q=" << (int)q << "\n";
	panic();
}

void cpu_t::id_execute_cb()
{
	reg8 instr = read_mem();
	reg8 x = GET_X(instr);
	reg8 y = GET_Y(instr);
	reg8 z = GET_Z(instr);
	rot_e rot;
	reg8_e r = get_r(z);

	switch(x)
	{
	case 0x00:
		rot = get_rot(y);
		#if DEBUG_OUTPUT == 2
			std::cout << rot_e_tostring(rot) << " "
				<< reg8_e_tostring(r) << "\n";
		#endif
		rot_exec(rot, r);				// ROT-instructions
		if(get_r(z) == _HL_)
			cycle(16);
		else
			cycle(8);
		return;
	case 0x01:
		#if DEBUG_OUTPUT == 2
			std::cout << "BIT " << std::hex << (int)y
				<< reg8_e_tostring(r) << "\n";
		#endif
		bit(y, r);						// BIT b,r
		if(get_r(z) == _HL_)
			cycle(16);
		else
			cycle(8);
		return;
	case 0x02:
		#if DEBUG_OUTPUT == 2
			std::cout << "RES " << std::hex << (int)y
				<< reg8_e_tostring(r) << "\n";
		#endif
		res(y, r);			   			// RES b,r
		if(get_r(z) == _HL_)
			cycle(16);
		else
			cycle(8);
		return;
	case 0x03:
		#if DEBUG_OUTPUT == 2
			std::cout << "SET " << std::hex << (int)y
				<< reg8_e_tostring(r) << "\n";
		#endif
		set(y,r);			   			// SET b,r
		if(get_r(z) == _HL_)
			cycle(16);
		else
			cycle(8);
		return;
	default:	break;
	}

	std::cout << "Unknown CB instruction 0x" << std::hex << (int)instr
		<< " at adr 0x" << std::hex << (int)(*get_reg(PC))-1 << "\n"
		<< "x=" << (int)x << " y=" << (int)y << " z=" << (int)z << "\n";
	panic();
}

void cpu_t::alu_exec(const alu_e op, const reg8 c)
{
	switch(op)
	{
		case ADD_A:	add(c);  return;
		case ADC_A:	adc(c);  return;
		case SUB:	sub(c);  return;
		case SBC_A:	sbc(c);  return;
		case AND:	_and(c); return;
		case XOR:	_xor(c); return;
		case OR:	_or(c);  return;
		case CP:	cp(c);   return;
	}
}

void cpu_t::alu_exec(const alu_e op, const reg8_e reg)
{
	if(reg == _HL_)
	{
		reg8 hldata = membus->read(*get_reg(HL));
		alu_exec(op, hldata);
		return;
	}

	alu_exec(op, *get_reg(reg));
}

void cpu_t::rot_exec(const rot_e op, const reg8_e reg)
{
	switch(op)
	{
		case RLC:	(reg == _HL_ ? rlchl() : rlc(reg));	return;
		case RRC:	(reg == _HL_ ? rrchl() : rrc(reg));	return;
		case RL:	(reg == _HL_ ? rlhl()  : rl(reg));	return;
		case RR:	(reg == _HL_ ? rrhl()  : rr(reg));	return;
		case SLA:	(reg == _HL_ ? slahl() : sla(reg));	return;
		case SRA:	(reg == _HL_ ? srahl() : sra(reg));	return;
		case SLL:	(reg == _HL_ ? swaphl(): swap(reg));return;
		case SRL:	(reg == _HL_ ? srlhl() : srl(reg));	return;
		default:	return;
	}
}

void cpu_t::add(const reg8 src)
{
	reg8 result = *get_reg(A) + src;
	if(result == 0x00)
		*get_reg(F) |= FLAG_Z;
	else
		*get_reg(F) &= ~FLAG_Z;
	*get_reg(F) &= ~FLAG_N;
	if((*get_reg(A) & 0x08) && (src & 0x08))
		*get_reg(F) |= FLAG_H;
	if((*get_reg(A) & 0x80) && (src & 0x80))
		*get_reg(F) |= FLAG_C;
	*get_reg(A) = result;
}

void cpu_t::adc(const reg8 src)
{
	int result = *get_reg(A) + src;
	if(*get_reg(F) & FLAG_C)
		result += 0x01;
	if((result & 0xFF) == 0)
		*get_reg(F) |= FLAG_Z;
	else
		*get_reg(F) &= ~FLAG_Z;
	*get_reg(F) &= ~FLAG_N;
	if((*get_reg(A) & 0x08) && (src & 0x08))
		*get_reg(F) |= FLAG_H;
	else
		*get_reg(F) &= ~FLAG_H;
	if(result > 0xFF)
		*get_reg(F) |= FLAG_C;
	else
		*get_reg(F) &= ~FLAG_C;
	*get_reg(A) = result;
}

void cpu_t::_and(const reg8 src)
{
	*get_reg(A) &= src;
	if(*get_reg(A) == 0x00)
		*get_reg(F) |= FLAG_Z;
	else
		*get_reg(F) &= ~FLAG_Z;
	*get_reg(F) &= ~FLAG_N;
	*get_reg(F) |= FLAG_H;
	*get_reg(F) &= ~FLAG_C;
}

void cpu_t::cp(const reg8 src)
{
	reg8 result = *get_reg(A);
	sub(src);	// sets right flags
	*get_reg(A) = result;
}

void cpu_t::_or(const reg8 src)
{
	*get_reg(A) |= src;
	if(*get_reg(A) == 0x00)
		*get_reg(F) |= FLAG_Z;
	else
		*get_reg(F) &= ~FLAG_Z;
	*get_reg(F) &= ~FLAG_N;
	*get_reg(F) &= ~FLAG_H;
	*get_reg(F) &= ~FLAG_Z;
}

void cpu_t::sub(const reg8 src)
{
	reg8 result = *get_reg(A) - src;
//	if(*get_reg(F) & FLAG_C)
//		result -= 0x80;
	if((result ) == 0x00)
		*get_reg(F) |= FLAG_Z;
	else
		*get_reg(F) &= ~FLAG_Z;

	*get_reg(F) |= FLAG_N;
	if((*get_reg(A) & 0x10) || !((*get_reg(A) & 0x10) && src))
		*get_reg(F) |= FLAG_H;
	else
		*get_reg(F) &= ~FLAG_H;
	if(result > *get_reg(A))
		*get_reg(F) |= FLAG_C;
	else
		*get_reg(F) &= ~FLAG_C;
	*get_reg(A) = result;
}

void cpu_t::sbc(const reg8 src)
{
	reg8 result = *get_reg(A) - src;
	if(result == 0x00)
		*get_reg(F) |= FLAG_Z;
	else
		*get_reg(F) &= ~FLAG_Z;
	*get_reg(F) |= FLAG_N;
	if((*get_reg(A) & 0x10) || !((*get_reg(A) & 0x10) && src))
		*get_reg(F) |= FLAG_H;
	else
		*get_reg(F) &= ~FLAG_H;
	if(!(*get_reg(A) & 0x80) && ((src & 0x80)
									|| (*get_reg(F) & FLAG_C)))
		*get_reg(F) |= FLAG_C;
	else
		*get_reg(F) &= ~FLAG_C;
	*get_reg(A) = result;
}

void cpu_t::_xor(const reg8 src)
{
	*get_reg(A) ^= src;
	if(*get_reg(A) == 0x00)
		*get_reg(F) |= FLAG_Z;
	else
		*get_reg(F) &= ~FLAG_Z;
	*get_reg(F) &= ~FLAG_N;
	*get_reg(F) &= ~FLAG_H;
	*get_reg(F) &= ~FLAG_Z;
}

void cpu_t::dec(const reg8_e dest)
{
	reg8 result =
		(dest == _HL_ ? membus->read(*get_reg(HL)) : (*get_reg(dest)));
	result--;

	if((result & 0xFF) == 0x00)
		*get_reg(F) |= FLAG_Z;
	else
		*get_reg(F) &= ~FLAG_Z;

	*get_reg(F) |= FLAG_N;
	if((result & 0x0F) == 0x0F)
		*get_reg(F) |= FLAG_H;
	else
		*get_reg(F) &= ~FLAG_H;

	if(dest == _HL_)
		membus->write(*get_reg(HL), result);
	else
		*get_reg(dest) = result;
}

void cpu_t::inc(const reg8_e dest)
{
	reg8 result =
		(dest == _HL_ ? membus->read(*get_reg(HL)) : (*get_reg(dest)));
	result++;

	if(result == 0x00)
		*get_reg(F) |= FLAG_Z;
	else
		*get_reg(F) &= ~FLAG_Z;

	*get_reg(F) &= ~FLAG_N;
	if(*get_reg(dest) & 0x08)
		*get_reg(F) |= FLAG_H;
	else
		*get_reg(F) &= ~FLAG_H;

	if(dest == _HL_)
		membus->write(*get_reg(HL), result);
	else
		*get_reg(dest) = result;
}

void cpu_t::swap(const reg8_e dest)
{
	reg8 temp = 0x00;
	if((*get_reg(dest) & 0x80) == 0x80)
		temp |= 0x01;
	if((*get_reg(dest) & 0x01) == 0x01)
		temp |= 0x08;

	*get_reg(dest) &= ~0x81;
	*get_reg(dest) |= temp;

	if((*get_reg(dest) & 0xFF) == 0x00)
		*get_reg(F) |= FLAG_Z;
	else
		*get_reg(F) &= ~FLAG_Z;
	*get_reg(F) &= ~FLAG_N;
	*get_reg(F) &= ~FLAG_H;
	*get_reg(F) &= ~FLAG_C;
}

void cpu_t::swaphl()
{
	reg8 temp = 0x00;
	reg8 data = membus->read(*get_reg(HL));
	if((data & 0x80) == 0x80)
		temp |= 0x01;
	if((data & 0x01) == 0x01)
		temp |= 0x08;

	data &= ~0x81;
	data |= temp;
	membus->write(*get_reg(HL), data);

	if((data & 0xFF) == 0x00)
		*get_reg(F) |= FLAG_Z;
	else
		*get_reg(F) &= ~FLAG_Z;
	*get_reg(F) &= ~FLAG_N;
	*get_reg(F) &= ~FLAG_H;
	*get_reg(F) &= ~FLAG_C;
}

void cpu_t::daa()
{
	reg8 result = *get_reg(A);
	reg8 acc_hi = result & 0x0F;
	reg8 acc_lo = result & 0xF0;
	reg8 f_n = *get_reg(F) & FLAG_N;
	reg8 f_c = *get_reg(F) & FLAG_C;
	reg8 f_h = *get_reg(F) & FLAG_H;
	reg8 new_flags = 0x00;
	if((f_n | f_c) == 0x00)
	{
		if(!f_h)
		{
			if(acc_hi < 0x0A && acc_lo < 0xA0)
			{
				result += 0x00;
			}
			else if(acc_hi < 0x09 && acc_lo > 0x90)
			{
				result += 0x06;
			}
			else if(acc_hi > 0x09 && acc_lo < 0xA0)
			{
				result += 0x60;
				new_flags |= FLAG_C;
			}
			else if(acc_hi > 0x08 && acc_lo > 0x90)
			{
				result += 0x66;
				new_flags |= FLAG_C;
			}
		}
		else if(f_h)
		{
			if(acc_hi < 0x0A && acc_lo < 0x40)
			{
				result += 0x06;
			}
			else if(acc_hi > 0x09 && acc_lo < 0x40)
			{
				result += 0x60;
				new_flags |= FLAG_C;
			}
		}
	}
	else if((f_n | f_c) == FLAG_C)
	{
		if(!f_h && acc_hi < 0x03 && acc_lo < 0xA0)
		{
			result += 0x60;
			new_flags |= FLAG_C;
		}
		else if(!f_h && acc_hi < 0x03 && acc_lo > 0x90)
		{
			result += 0x66;
			new_flags |= FLAG_C;
		}
		else if(f_h && acc_hi < 0x03 && acc_lo < 0x3)
		{
			result += 0x66;
			new_flags |= FLAG_C;
		}
	}
	else if((f_n | f_c) == FLAG_N)
	{
		if(!f_h && acc_hi < 0x0A && acc_lo < 0xA0)
		{
			result += 0x00;
		}
		if(f_h && acc_hi < 0x09 && acc_lo > 0x50)
		{
			result += 0xFA;
		}
	}
	else if((f_n | f_c) == (FLAG_N & FLAG_C))
	{
		if(!f_h && acc_hi > 0x06 && acc_lo < 0xA0)
		{
			result += 0xA0;
			new_flags |= FLAG_C;
		}
		else if(f_h && acc_hi > 0x05 && acc_lo > 0x50)
		{
			result += 0x9A;
			new_flags |= FLAG_C;
		}
	}
	if(result == 0x00)
		new_flags |= FLAG_Z;
	else
		new_flags &= ~FLAG_Z;
	if(f_n)
		new_flags |= FLAG_N;
	else
		new_flags &= ~FLAG_N;
	*get_reg(A) = result;
	*get_reg(F) = new_flags;
}

void cpu_t::cpl()
{
	*get_reg(A) = *get_reg(A);
	*get_reg(F) |= FLAG_N;
	*get_reg(F) |= FLAG_H;
}

void cpu_t::ccf()
{
	if((*get_reg(F) & FLAG_C) == FLAG_C)
		*get_reg(F) &= ~FLAG_C;
	else
		*get_reg(F) |= FLAG_C;

	*get_reg(F) &= ~FLAG_N;
	*get_reg(F) &= ~FLAG_H;
}

void cpu_t::scf()
{

	*get_reg(F) |= FLAG_C;
	*get_reg(F) &= ~FLAG_N;
	*get_reg(F) &= ~FLAG_H;
}

void cpu_t::nop()
{
//	panic();
}

void cpu_t::halt()
{
	std::cout << "Halt\n";
	halted = true;
}

void cpu_t::stop()
{
	std::cout << "STOP!\n";
	panic();
}

void cpu_t::di()
{
	IME = false;
}

void cpu_t::ei()
{
	IME = true;
}

void cpu_t::ld(const reg8_e dest, const reg8_e src)
{
	switch(dest)
	{
		case _HL_: membus->write(*get_reg(HL), *get_reg(src));			return;
		case _C_:  membus->write(0xFF00 + *get_reg(C), *get_reg(src));	return;
		case _BC_: membus->write(*get_reg(BC), *get_reg(src));			return;
		case _DE_: membus->write(*get_reg(DE), *get_reg(src));			return;
		default: break;
	}
	switch(src)
	{
		case _HL_: *get_reg(dest) = membus->read(*get_reg(HL));			return;
		case _C_:  *get_reg(dest) = membus->read(0xFF00 + *get_reg(C));	return;
		case _BC_: *get_reg(dest) = membus->read(*get_reg(BC));			return;
		case _DE_: *get_reg(dest) = membus->read(*get_reg(DE));			return;
		default: break;
	}

	*get_reg(dest) = *get_reg(src);
}

void cpu_t::ld(const reg8_e dest, const reg8 src)
{
	switch(dest)
	{
		case _HL_: membus->write(*get_reg(HL), src);			return;
		case _C_:  membus->write(0xFF00 + *get_reg(C), src);	return;
		case _BC_: membus->write(*get_reg(BC), src);			return;
		case _DE_: membus->write(*get_reg(DE), src);			return;
		default: break;
	}

	*get_reg(dest) = src;
}

void cpu_t::ld(const reg16_e dest, const reg16_2x8 src)
{
	*get_reg(dest) = src.r16;
}

void cpu_t::ldsphl()
{
	*get_reg(SP) = *get_reg(HL);
}

void cpu_t::ldabc()
{
	*get_reg(A) = membus->read(*get_reg(BC));
}
void cpu_t::ldbca()
{
	membus->write(*get_reg(BC), *get_reg(A));
}
void cpu_t::ldade()
{
	*get_reg(A) = membus->read(*get_reg(DE));
}
void cpu_t::lddea()
{
	membus->write(*get_reg(DE), *get_reg(A));
}
void cpu_t::ldahl()
{
	*get_reg(A) = membus->read(*get_reg(HL));
}
void cpu_t::ldhla()
{
	membus->write(*get_reg(HL), *get_reg(A));
}

void cpu_t::lddahl()
{
	*get_reg(A) = membus->read((*get_reg(HL))--);
}
void cpu_t::lddhla()
{
	membus->write((*get_reg(HL))--, *get_reg(A));
}

void cpu_t::ldiahl()
{
	*get_reg(A) = membus->read((*get_reg(HL))++);
}

void cpu_t::ldihla()
{
	//std::cout << "PC: " << std::hex << (int)*get_reg(PC) << "\n";
	membus->write((*get_reg(HL))++, *get_reg(A));
}

void cpu_t::ldhna_byte(const int8_t n)
{
	ldhna_word(0xFF00 + n);
}

void cpu_t::ldhan_byte(const int8_t n)
{
	ldhan_word(0xFF00 + n);
}

void cpu_t::ldhlspn_byte(const int8_t n)
{
	*get_reg(A) = membus->read(*get_reg(SP) + n);
}

void cpu_t::ldhna_word(const reg16 n)
{
	membus->write(n, *get_reg(A));
}

void cpu_t::ldhan_word(const reg16 n)
{
	*get_reg(A) = membus->read(n);
}

void cpu_t::ldhnnsp(const reg16 n)
{
	membus->write(n, *get_reg(SP));
}

void cpu_t::ldhspnn(const reg16 n)
{
	*get_reg(A) = membus->read(n);
}

void cpu_t::rlca()
{
	rlc(A);
}

void cpu_t::rlchl()
{
	reg8 adata = *get_reg(A);
	ldahl();
	rlc(A);
	ldhla();
	*get_reg(A) = adata;
}

void cpu_t::rlhl()
{
	reg8 adata = *get_reg(A);
	ldahl();
	rl(A);
	ldhla();
	*get_reg(A) = adata;
}

void cpu_t::rrchl()
{
	reg8 adata = *get_reg(A);
	ldahl();
	rrc(A);
	ldhla();
	*get_reg(A) = adata;
}

void cpu_t::rrhl()
{
	reg8 adata = *get_reg(A);
	ldahl();
	rr(A);
	ldhla();
	*get_reg(A) = adata;
}

void cpu_t::slahl()
{
	reg8 adata = *get_reg(A);
	ldahl();
	sla(A);
	ldhla();
	*get_reg(A) = adata;
}

void cpu_t::srahl()
{
	reg8 adata = *get_reg(A);
	ldahl();
	sra(A);
	ldhla();
	*get_reg(A) = adata;
}

void cpu_t::srlhl()
{
	reg8 adata = *get_reg(A);
	ldahl();
	srl(A);
	ldhla();
	*get_reg(A) = adata;
}

void cpu_t::rlc(const reg8_e dest)
{
	reg8 cdata = (*get_reg(dest) & 0x80) ? 0x01 : 0x00;
	if(cdata)
		*get_reg(F) |= FLAG_C;

	*get_reg(dest) = *get_reg(dest) << 1;
	*get_reg(dest) |= cdata;

	if(*get_reg(dest) == 0x00)
		*get_reg(F) |= FLAG_Z;
	else
		*get_reg(F) &= ~FLAG_Z;
	*get_reg(F) &= ~FLAG_N;
	*get_reg(F) &= ~FLAG_H;
}

void cpu_t::rla()
{
	rl(A);
}

void cpu_t::rl(const reg8_e dest)
{
	reg8 cdata = (*get_reg(F) & FLAG_C) ? 0x01 : 0x00;
	if(*get_reg(dest) & 0x80)
		*get_reg(F) |= FLAG_C;
	else
		*get_reg(F) &= ~FLAG_C;

	*get_reg(dest) = *get_reg(dest) << 1;
	*get_reg(dest) |= cdata;

	if(*get_reg(dest) == 0x00)
		*get_reg(F) |= FLAG_Z;
	else
		*get_reg(F) &= ~FLAG_Z;

	*get_reg(F) &= ~FLAG_N;
	*get_reg(F) &= ~FLAG_H;
}

void cpu_t::rrca()
{
	rrc(A);
}

void cpu_t::rrc(const reg8_e dest)
{
	reg8 cdata = (*get_reg(dest) & 0x01) ? 0x80 : 0x00;
	if(cdata)
		*get_reg(F) |= FLAG_C;
	else
		*get_reg(F) &= ~FLAG_C;

	*get_reg(dest) = *get_reg(dest) >> 1;
	*get_reg(dest) |= cdata;

	if(*get_reg(dest) == 0x00)
		*get_reg(F) |= FLAG_Z;
	else
		*get_reg(F) &= ~FLAG_Z;

	*get_reg(F) &= ~FLAG_N;
	*get_reg(F) &= ~FLAG_H;
}

void cpu_t::rra()
{
	rr(A);
}

void cpu_t::rr(const reg8_e dest)
{
	reg8 cdata = (*get_reg(F) & FLAG_C) ? 0x80 : 0x00;
	if(*get_reg(dest) & 0x01)
		*get_reg(F) |= FLAG_C;
	else
		*get_reg(F) &= ~FLAG_C;

	*get_reg(dest) = *get_reg(dest) >> 1;
	*get_reg(dest) |= cdata;

	if(*get_reg(dest) == 0x00)
		*get_reg(F) |= FLAG_Z;
	else
		*get_reg(F) &= ~FLAG_Z;
	*get_reg(F) &= ~FLAG_N;
	*get_reg(F) &= ~FLAG_H;
}

void cpu_t::sla(const reg8_e dest)
{
	if(*get_reg(dest) & 0x80)
		*get_reg(F) |= FLAG_C;
	else
		*get_reg(F) &= ~FLAG_C;

	*get_reg(dest) = *get_reg(dest) << 1;
	if(*get_reg(dest) == 0x00)
		*get_reg(F) |= FLAG_Z;
	else
		*get_reg(F) &= ~FLAG_Z;
	*get_reg(F) &= ~FLAG_N;
	*get_reg(F) &= ~FLAG_H;
}

void cpu_t::sra(const reg8_e dest)
{
	reg8 msbdata = *get_reg(dest) & 0x80;
	if(*get_reg(dest) & 0x01)
		*get_reg(F) |= FLAG_C;
	else
		*get_reg(F) &= ~FLAG_C;

	*get_reg(dest) = *get_reg(dest) >> 1;
	*get_reg(dest) |= msbdata;

	if(*get_reg(dest) == 0x00)
		*get_reg(F) |= FLAG_Z;
	else
		*get_reg(F) &= ~FLAG_Z;
	*get_reg(F) &= ~FLAG_N;
	*get_reg(F) &= ~FLAG_H;
}

void cpu_t::srl(const reg8_e dest)
{
	if(*get_reg(dest) & 0x01)
		*get_reg(F) |= FLAG_C;
	else
		*get_reg(F) &= ~FLAG_C;

	*get_reg(dest) = *get_reg(dest) >> 1;

	if(*get_reg(dest) == 0x00)
		*get_reg(F) |= FLAG_Z;
	else
		*get_reg(F) &= ~FLAG_Z;
	*get_reg(F) &= ~FLAG_N;
	*get_reg(F) &= ~FLAG_H;
}

void cpu_t::addhl(reg16_e src)
{
	reg16 result = *get_reg(HL) + *get_reg(src);
	*get_reg(F) &= ~FLAG_N;
	if((*get_reg(HL) & 0x0800) && (*get_reg(src) & 0x0800))
		*get_reg(F) |= FLAG_H;
	else
		*get_reg(F) &= ~FLAG_H;
	if((*get_reg(HL) & 0x8000) && (*get_reg(src) & 0x8000))
		*get_reg(F) |= FLAG_C;
	else
		*get_reg(F) &= ~FLAG_C;
	*get_reg(HL) = result;
}

void cpu_t::addsp(const reg8 src)
{
	reg16 result = *get_reg(SP) + src;
	*get_reg(F) &= ~FLAG_Z;
	*get_reg(F) &= ~FLAG_N;
	if((*get_reg(SP) & 0x0800) && (src & 0x0800))
		*get_reg(F) |= FLAG_H;
	else
		*get_reg(F) &= ~FLAG_H;
	if((*get_reg(SP) & 0x8000) && (src & 0x8000))
		*get_reg(F) |= FLAG_C;
	else
		*get_reg(F) &= ~FLAG_C;
	*get_reg(SP) = result;
}

void cpu_t::inc(reg16_e dest)
{
	(*get_reg(dest))++;
}
void cpu_t::dec(reg16_e dest)
{
	(*get_reg(dest))--;
}

void cpu_t::bit(const reg8 b, const reg8_e src)
{
	reg8 val;
	if(src == _HL_)
		val = membus->read(*get_reg(HL));
	else
		val = *get_reg(src);

	unsigned char c = 0x00;
	switch(b)
	{
		case 0: c = 0x01;	break;
		case 1: c = 0x02;	break;
		case 2: c = 0x04;	break;
		case 3: c = 0x08;	break;
		case 4: c = 0x10;	break;
		case 5: c = 0x20;	break;
		case 6: c = 0x40;	break;
		case 7: c = 0x80;	break;
	}
	if(((val & c) & 0xFF) == 0x00)
		*get_reg(F) |= FLAG_Z;
	else
		*get_reg(F) &= ~FLAG_Z;
	*get_reg(F) &= ~FLAG_N;
	*get_reg(F) |= FLAG_H;
}

void cpu_t::set(const reg8 b, const reg8_e src)
{
	unsigned char c = 0x00;
	switch(b)
	{
		case 0: c = 0x01;	break;
		case 1: c = 0x02;	break;
		case 2: c = 0x04;	break;
		case 3: c = 0x08;	break;
		case 4: c = 0x10;	break;
		case 5: c = 0x20;	break;
		case 6: c = 0x40;	break;
		case 7: c = 0x80;	break;
	}

	if(src == _HL_)
		membus->write(*get_reg(HL), (membus->read(*get_reg(HL)) | c));
	else
		*get_reg(src) |= c;
}

void cpu_t::res(const reg8 b, const reg8_e src)
{
	unsigned char c = 0x00;
	switch(b)
	{
		case 0: c = 0x01;	break;
		case 1: c = 0x02;	break;
		case 2: c = 0x04;	break;
		case 3: c = 0x08;	break;
		case 4: c = 0x10;	break;
		case 5: c = 0x20;	break;
		case 6: c = 0x40;	break;
		case 7: c = 0x80;	break;
	}

	if(src == _HL_)
		membus->write(*get_reg(HL), (membus->read(*get_reg(HL)) & ~c));
	else
		*get_reg(src) &= ~c;
}

#define C_NZ	((*get_reg(F) & FLAG_Z) == 0x00)
#define C_Z		((*get_reg(F) & FLAG_Z) == FLAG_Z)
#define	C_NC	((*get_reg(F) & FLAG_C) == 0x00)
#define C_CC	(*get_reg(F) & FLAG_C)

void cpu_t::jp(const cond_e c, const uint16_t d)
{
	switch(c)
	{
		case NZ:	if(C_NZ)	jp(d);	return;
		case Z:		if(C_Z)		jp(d);	return;
		case NC:	if(C_NC)	jp(d);	return;
		case CC:	if(C_CC)	jp(d);	return;
		case PO:
		case PE:
		case P:
		case M:
		default:	panic();			return;
	}
}

void cpu_t::jp(const uint16_t d)
{
	*get_reg(PC) = d;
}

void cpu_t::jphl()
{
	jp(*get_reg(HL));
}

void cpu_t::jr(const cond_e c, const int8_t d)
{
	switch(c)
	{
		case NZ:	if(C_NZ)	jr(d);	return;
		case Z:		if(C_Z)		jr(d);	return;
		case NC:	if(C_NC)	jr(d);	return;
		case CC:	if(C_CC)	jr(d);	return;
		case PO:
		case PE:
		case P:
		case M:
		default:	panic();			return;
	}
}

void cpu_t::jr(const int8_t d)
{
	*get_reg(PC) += d;
}

void cpu_t::push(const reg16_e nn)
{
	reg16_2x8 data;
	data.r16 = *get_reg(nn);

	membus->write(--(*get_reg(SP)), data.r8.h);
	membus->write(--(*get_reg(SP)), data.r8.l);

}

void cpu_t::pop(const reg16_e nn)
{
	reg16_2x8 data;
	data.r8.l = membus->read((*get_reg(SP))++);
	data.r8.h = membus->read((*get_reg(SP))++);

	*get_reg(nn) = data.r16;
}

void cpu_t::call(const reg16 nn)
{
	push(PC);
	*get_reg(PC) = nn;
}

void cpu_t::call(const cond_e c, const reg16 nn)
{
	switch(c)
	{
		case NZ:	if(C_NZ)	call(nn);	return;
		case Z:		if(C_Z)		call(nn);	return;
		case NC:	if(C_NC)	call(nn);	return;
		case CC:	if(C_CC)	call(nn);	return;
		case PO:
		case PE:
		case P:
		case M:
		default:	panic();				return;
	}
}


void cpu_t::rst(const reg8 n)
{
	push(PC);
	jp(0x0000 + n);
}

void cpu_t::ret()
{
	pop(PC);
}

void cpu_t::ret(const cond_e c)
{
	switch(c)
	{
		case NZ:	if(C_NZ)	ret();	return;
		case Z:		if(C_Z)		ret();	return;
		case NC:	if(C_NC)	ret();	return;
		case CC:	if(C_CC)	ret();	return;
		case PO:
		case PE:
		case P:
		case M:
		default:	panic();			return;
	}
}

void cpu_t::reti()
{
	ret();
	IME = true;
}

std::string reg8_e_tostring(const reg8_e r)
{
	switch(r)
	{
		case A: return "A";
		case B: return "B";
		case C: return "C";
		case D: return "D";
		case E: return "E";
		case F: return "F";
		case H: return "H";
		case L: return "L";
		case _HL_:return "(HL)";
		default:return "?";
	}
}

std::string reg16_e_tostring(const reg16_e r)
{
	switch(r)
	{
		case AF: return "AF";
		case BC: return "BC";
		case DE: return "DE";
		case HL: return "HL";
		case SP: return "SP";
		case PC: return "PC";
		default: return "??";
	}
}

std::string cond_e_tostring(const cond_e cc)
{
	switch(cc)
	{
		case NZ:	return "NZ";
		case Z:		return "Z";
		case NC:	return "NC";
		case CC:	return "CC";
		case PO:	return "PO";
		case PE:	return "PE";
		case P:		return "P";
		case M:		return "M";
	}
	return "cond_e_?";
}

std::string alu_e_tostring(const alu_e alu)
{
	switch(alu)
	{
		case ADD_A:	return "ADD";
		case ADC_A:	return "ADC";
		case SUB:	return "SUB";
		case SBC_A:	return "SBC";
		case AND:	return "AND";
		case XOR:	return "XOR";
		case OR:	return "OR";
		case CP:	return "CP";
	}
	return "alu_e_?";
}

std::string rot_e_tostring(const rot_e rot)
{
	switch(rot)
	{
		case RLC:	return "RLC";
		case RRC:	return "RRC";
		case RL:	return "RL";
		case RR:	return "RR";
		case SLA:	return "SLA";
		case SRA:	return "SRA";
		case SLL:	return "SLL";
		case SRL:	return "SRL";
	}
	return "rot_e_?";
}

std::string binstring(const unsigned char byte)
{
	std::string s;
	int i, j;
	for(i = 0, j = 0x80; i < 8; ++i)
	{
		s += (byte & j ? '1' : '0');
		j >>= 1;
	}
	return s;
}

std::string binstring(const unsigned short bytes)
{
	std::string s;
	int i, j;
	for(i = 0, j = 0x8000; i < 16; ++i)
	{
		s += (bytes & j ? '1' : '0');
		j >>= 1;
	}
	return s;
}

void cpu_t::print()
{
	std::cout << "[Registers]\nR dec hex bin\n";
	for(int i = 0; i < 8; ++i)
	{
		reg8_e r = (reg8_e)i;
		std::cout << reg8_e_tostring(r) << " ";
		std::cout << std::setw(3) << (int)(*get_reg(r)) << ", ";
		std::cout << std::hex << std::setw(3) << (int)*get_reg(r);
		std::cout << std::dec << ", " << binstring(*get_reg(r)) << "\n";
	}
	std::cout << "SP " << std::setw(4) << (int)(*get_reg(SP)) << ", ";
	std::cout << std::hex << std::setw(4) << (int)*get_reg(SP);
	std::cout << std::dec << ", " << binstring(*get_reg(SP)) << "\n";

	std::cout << "PC " << std::setw(4) << (int)(*get_reg(PC)) << ", ";
	std::cout << std::hex << std::setw(4) << (int)*get_reg(PC);
	std::cout << std::dec << ", " << binstring(*get_reg(PC)) << "\n";

	std::cout << "[Flags]\nZNHC\n" << binstring(*get_reg(F)) << "\n";
}

void cpu_t::inject_code(uint8_t *code, size_t length, reg16 new_pc, int steps)
{
	// Disable bootrom to make sure code is injected in ROM, not BOOTROM.
	bool reenable_bootrom = !booted;
	membus->disable_bootrom();

	reg16 old_pc = *get_reg(PC);
	int8_t *old_code = new int8_t[length];

	memcpy(old_code, membus->get_pointer(new_pc), length);
	memcpy(membus->get_pointer(new_pc), code, length);

	*get_reg(PC) = new_pc;
	for(int i = 0; i < steps && !panicked; ++i)
	{
		run();
	}
	if(steps == 0 && !panicked)
	{
		do{run();}
		while(last_instr != 0x00 && !panicked);
	}

	memcpy(membus->get_pointer(old_pc), old_code, length);
	*get_reg(PC) = old_pc;
	delete old_code;

	if(reenable_bootrom)
		membus->enable_bootrom();
}

void cpu_t::panic()
{
	std::cout << "========\nCPU panicked!\n";
	std::cout << "Last instruction 0x" << std::hex << (int)last_instr
		<< " at adr 0x" << std::hex << (int)last_adr << "\n";
	std::cout << "========\n";
	print();
	panicked = true;
}

void cpu_t::cycle(uint8_t n)
{
	cycles_left = n;
}

void cpu_t::set_flags(bool N, bool Z, bool H, bool C)
{
	*get_reg(F) = (N ? *get_reg(F) | FLAG_N : *get_reg(F) & ~FLAG_N);
	*get_reg(F) = (Z ? *get_reg(F) | FLAG_Z : *get_reg(F) & ~FLAG_Z);
	*get_reg(F) = (H ? *get_reg(F) | FLAG_H : *get_reg(F) & ~FLAG_H);
	*get_reg(F) = (C ? *get_reg(F) | FLAG_C : *get_reg(F) & ~FLAG_C);
}

bool cpu_t::is_panicked()
{
	return panicked;
}
