#include "./unit.h"

#include <stdint.h>
#include <string.h>

#include "opcode.h"
#include "emulator.h"

Emulator emu = {0};
char *registers_labels[] = {
	"b", "c", "d", "e",
	"h", "l", NULL, "a"
};
uint8_t *registers[] = {
	&emu.cpu.b, &emu.cpu.c, &emu.cpu.d, &emu.cpu.e, 
	&emu.cpu.h,&emu.cpu.l, NULL, &emu.cpu.a
};
int HL = 6;

int test_ld() {
	memset(&emu, 0, sizeof(Emulator));

	uint8_t target_opcode[] = { 0x40, 0x48, 0x50, 0x58, 0x60, 0x68, 0x70, 0x78 };
	for (int target = 0; target < 8; target++) {
	for (int i = 0; i < 8; i++) {
		if (i == HL || target == HL) continue;

		*registers[target] = 0;
		uint8_t value = i * 7;
		*registers[i] = value;

		uint8_t flags_before = emu.cpu.f;
		int opcode = target_opcode[target] + i;
		opcode_execute(&emu, opcode);
		
		assertm_eq(*registers[target], value, "%02X", "[OPCODE %x] LD b, %s", opcode, registers_labels[i]);
		assertm_eq(emu.cpu.f, flags_before, "%02X", "[OPCODE %02X] Flags got updated!", opcode);
	}}
	return SUCCESS;
}


#define TEST_ARITH(OPCODE, A_INIT, OPERATOR, LABEL) \
	memset(&emu, 0, sizeof(Emulator));\
	for (int i = 0; i < 8; i++) {\
		if (i == HL) continue;\
		emu.cpu.a = (A_INIT);\
      		emu.cpu.f = 0;\
		*registers[i] = i * 8;\
		uint8_t result = emu.cpu.a OPERATOR *registers[i];\
		int opcode = (OPCODE) + i;\
		opcode_execute(&emu, opcode);\
		assertm_eq(emu.cpu.a, result, "%02X", "[OPCODE %x] "LABEL" b, %s", opcode, registers_labels[i]);\
	}
int test_arith() {
	TEST_ARITH(0x80, 0x10, +, "ADD")
	TEST_ARITH(0x88, 0x12, +, "ADC")
	TEST_ARITH(0x90, 0x10, -, "SUB")
	TEST_ARITH(0x98, 0x12, -, "SBC")
	TEST_ARITH(0xA0, 0xFF, &, "AND")
	TEST_ARITH(0xA8, 0xFF, ^, "XOR")
	TEST_ARITH(0xB0, 0xFF, |, "OR")
	return SUCCESS;
}
#undef TEST_ARITH


int test_arith_flags() {
	const int Z = 1 << 7;
	const int N = 1 << 6;
	const int H = 1 << 5;
	const int C = 1 << 4;

	memset(&emu, 0, sizeof(Emulator));
	opcode_execute(&emu, 0x80);
	assertm_eq(emu.cpu.f, Z, "%02X", "ADD Z invalid flags!");

	// Full/Half carry
	emu.cpu.a = 0b10001000;
	emu.cpu.b = 0b10001000;
	opcode_execute(&emu, 0x80);
	assertm_eq(emu.cpu.f, H | C, "%02X", "ADC H C invalid flags!");

	// Full/Half carry ADC
	emu.cpu.f = C;
	emu.cpu.a = 0b10001000;
	emu.cpu.b = 0b10000111;
	opcode_execute(&emu, 0x88);
	assertm_eq(emu.cpu.f, H | C, "%02X", "ADC Full/Half carry not set!");

	// SUB
	emu.cpu.a = 0b10010000;
	emu.cpu.b = 0b10010001;
	opcode_execute(&emu, 0x90);
	assertm_eq(emu.cpu.f, N | H | C, "%02X", "SUB ZNHC invalid flags!");

	emu.cpu.f = C;
	emu.cpu.a = 0b10010000;
	emu.cpu.b = 0b10010000;
	opcode_execute(&emu, 0x98);
	assertm_eq(emu.cpu.f, N | H | C, "%02X", "SBC ZNHC invalid flags!");

	return SUCCESS;
}

int main() {
	TEST_SETUP();

	TEST_RUN(test_ld);
	TEST_RUN(test_arith);
	TEST_RUN(test_arith_flags);

	TEST_FINISH();
}

