#ifndef CPU_H
#define CPU_H

//#define DEBUG_OUTPUT 2
//#define BREAKPOINT 0x839

#include <iostream>
#include <iomanip>
#include <string>

#define FLAG_Z  0x80
#define FLAG_N  0x40
#define FLAG_H  0x20
#define FLAG_C  0x10

#include "membus.h"

typedef uint8_t reg8;
typedef uint16_t reg16;

union reg16_2x8
{
	struct r8_t
	{
		#if GB_BIG_ENDIAN
			reg8 h, l;
		#else
			reg8 l, h;
		#endif
	} r8;
	reg16 r16;
};

typedef struct {
	reg16 adr;
	reg8 instr;
	reg8 data8;
	reg16_2x8 data16;
} linstr_state_e;

typedef enum
{
	B,
	C,
	D,
	E,
	H,
	L,
	A,
	F,
	_HL_,	// (HL), memory adress in HL
	_C_,
	_BC_,
	_DE_
} reg8_e;

typedef enum
{
	AF,
	BC,
	DE,
	HL,
	SP,
	PC
} reg16_e;

typedef enum
{
	NZ,
	Z,
	NC,
	CC,
	PO,
	PE,
	P,
	M
} cond_e;

typedef enum
{
	ADD_A,
	ADC_A,
	SUB,
	SBC_A,
	AND,
	XOR,
	OR,
	CP
} alu_e;

typedef enum
{
	RLC,
	RRC,
	RL,
	RR,
	SLA,
	SRA,
	SLL,
	SRL
} rot_e;

class cpu_t
{
	private:
	linstr_state_e last_instr;
	bool booted;
	bool panicked;
	bool halted;
	bool IME;
	reg8 *IE;
	reg8 *IF;
	uint8_t cycles_left;

	reg16_2x8 registers[6];
	membus_t *membus;

	reg8 *get_reg(const reg8_e reg);
	reg16 *get_reg(const reg16_e reg);

	reg8 read_reg(const reg8_e reg);

	reg8_e get_r(const reg8 r);
	reg16_e get_rp(const reg8 r);
	reg16_e get_rp2(const reg8 r);
	cond_e get_cc(const reg8 r);
	alu_e get_alu(const reg8 r);
	rot_e get_rot(const reg8 r);

	void id_execute();
	void id_execute_cb();
	void alu_exec(const alu_e op, const reg8 c);
	void alu_exec(const alu_e op, const reg8_e reg);
	void rot_exec(const rot_e op, const reg8_e reg);

	int8_t read_mem();
	void check_interrupts();
	void inc_counters();

	void ld(const reg8_e dest, const reg8_e src);
	void ld(const reg8_e dest, const reg8 src);
	void ld(const reg16_e dest, const reg16_2x8 src);
	void ldsphl();
	void ldabc();
	void ldbca();
	void ldade();
	void lddea();
	void ldahl();
	void ldhla();
	void lddahl();
	void lddhla();
	void ldiahl();
	void ldihla();
	void ldhna_byte(const uint8_t n);
	void ldhan_byte(const uint8_t n);
	void ldhlspn_byte(const uint8_t n);
	void ldhna_word(const reg16 n);
	void ldhan_word(const reg16 n);
	void ldhnnsp(const reg16 n);
	void ldhspnn(const reg16 n);

	void add(const reg8 src);
	void adc(const reg8 src);
	void _and(const reg8 src);
	void cp(const reg8 src);
	void _or(const reg8 src);
	void sub(const reg8 src);
	void sbc(const reg8 src);
	void _xor(const reg8 src);

	void add(const reg8_e src);
	void adc(const reg8_e src);
	void _and(const reg8_e src);
	void cp(const reg8_e src);
	void _or(const reg8_e src);
	void sub(const reg8_e src);
	void sbc(const reg8_e src);
	void _xor(const reg8_e src);

	void dec(const reg8_e dest);
	void inc(const reg8_e dest);

	void swap(const reg8_e dest);
	void swaphl();
	void daa();
	void cpl();
	void ccf();
	void scf();
	void nop();
	void halt();
	void stop();
	void di();
	void ei();

	void rlca();
	void rla();
	void rrca();
	void rra();
	void rlchl();
	void rlhl();
	void rrchl();
	void rrhl();
	void slahl();
	void srahl();
	void srlhl();
	void rlc(const reg8_e dest);
	void rl(const reg8_e dest);
	void rrc(const reg8_e dest);
	void rr(const reg8_e dest);
	void sla(const reg8_e dest);
	void sra(const reg8_e dest);
	void sll(const reg8_e dest);
	void srl(const reg8_e dest);

	void addhl(const reg16_e src);
	void addsp(const reg8 src);
	void inc(const reg16_e dest);
	void dec(const reg16_e dest);

	void bit(const reg8 b, const reg8_e src);
	void set(const reg8 b, const reg8_e src);
	void res(const reg8 b, const reg8_e src);

	void jp(const cond_e c, const uint16_t d);
	void jp(const uint16_t d);
	void jphl();

	void jr(const cond_e c, const int8_t d);
	void jr(const int8_t d);

	void push(const reg16_e nn);
	void pop(const reg16_e nn);
	void call(const reg16 nn);
	void call(const cond_e c, const reg16 nn);

	void rst(const reg8 n);

	void ret();
	void ret(const cond_e c);
	void reti();

	void panic();
	void cycle(uint8_t n);

	void set_flags(bool, bool, bool, bool);

	public:
	void init(membus_t *membus_, bool bootrom_enabled);
	void run();
	void inject_code(uint8_t *code, size_t length, reg16 new_pc, int steps = 0);

	void print();
	bool is_panicked() const;
};

#include "cpu_debug.h"

#endif
