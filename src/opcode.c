#include "opcode.h"
#include "interrupts.h"
#include "logger.h"
#include "memory_map.h"
#include "timer.h"
#include <stdint.h>
#include <stdio.h>

#define LEN(c) emu->cpu.opcode_length = c
#define CYCLE(c) emu->cpu.cycles = c

#define MASK_FLAG emu->cpu.f &= 0xF0

#define SET_FLAG(Z, N, H, C) emu->cpu.f = ((Z) << 7) | ((N) << 6) | ((H) << 5) | ((C) << 4)

#define FLAG_C ((emu->cpu.f >> 4) & 1)
#define FLAG_H ((emu->cpu.f >> 5) & 1)
#define FLAG_N ((emu->cpu.f >> 6) & 1)
#define FLAG_Z ((emu->cpu.f >> 7) & 1)

static inline void prefix_opcodes(Emulator* emu);

void opcode_execute(Emulator* emu, uint8_t opcode) {
	switch (opcode) {
	case 0x00: LEN(1); CYCLE(4); break; // NOP
	case 0x10: // STOP
		LEN(2); CYCLE(4); 
		emu->cpu.is_stopped = true; 
		timer_div_reset(emu);
	break;
	case 0x76: // HALT
		LEN(1); CYCLE(4);
		if (!emu->interrupt.ime && interrupt_pending(emu) != 0) {
			emu->cpu.is_halt_bugged = true;
		} else {
			emu->cpu.is_halted = true;
		}
	break;
	case 0xF3: LEN(1); CYCLE(4); emu->interrupt.ime = false; break; // DI
	case 0xFB: LEN(1); CYCLE(4); emu->cpu.ime_scheduled = true; break; // EI
	case 0xCB: prefix_opcodes(emu); break;

	// ===========================
	// ========== STACK ==========
	// ===========================
	
	#define POP(TARGET, ...) \
		LEN(1); CYCLE(12); emu->cpu.TARGET = memory_read_16(emu, emu->cpu.sp); \
		emu->cpu.sp += 2; __VA_ARGS__; break

	#define PUSH(SOURCE) \
		LEN(1); CYCLE(16); emu->cpu.sp -= 2; \
		memory_write_16(emu, emu->cpu.sp, emu->cpu.SOURCE); \
		break
	
	case 0xC1: POP(bc);
	case 0xD1: POP(de);
	case 0xE1: POP(hl);
	case 0xF1: POP(af, MASK_FLAG);

	case 0xC5: PUSH(bc);
	case 0xD5: PUSH(de);
	case 0xE5: PUSH(hl);
	case 0xF5: PUSH(af);

	// ========================
	// ========== LD16 ==========
	// ========================
		
	#define LD16_d16(TARGET) \
		LEN(3); CYCLE(12); emu->cpu.TARGET = memory_read_16(emu, emu->cpu.pc + 1); \
		break
	// LD16
	case 0x01: LD16_d16(bc);
	case 0x11: LD16_d16(de);
	case 0x21: LD16_d16(hl);
	case 0x31: LD16_d16(sp);
	case 0x08: // LD (a16), SP
		LEN(3); CYCLE(20);
		memory_write_16(emu, memory_read_16(emu, emu->cpu.pc + 1), emu->cpu.sp);
		break;
	case 0xF8: { // LD HL, SP+r8
		LEN(2); CYCLE(12);
		int8_t offset = (int8_t)memory_read(emu, emu->cpu.pc + 1);
		uint8_t uoffset = (uint8_t)offset;
		uint8_t sp_low = emu->cpu.sp & 0xFF;
		bool hc = (sp_low & 0xF) + (uoffset & 0xF) > 0xF;
		bool c = (sp_low + uoffset) > 0xFF;
		SET_FLAG(0, 0, hc, c);
		emu->cpu.hl = emu->cpu.sp + offset;
	} break;
	case 0xF9: // LD SP, HL
		LEN(1); CYCLE(8);
		emu->cpu.sp = emu->cpu.hl;
		break;
	
	// ========================
	// ========== LD ==========
	// ========================

	case 0xE0: { // LDH (a8), A
		LEN(2); CYCLE(12);
		uint16_t address = 0xFF00 + (uint16_t)memory_read(emu, emu->cpu.pc + 1);
		memory_write(emu, address, emu->cpu.a);
	} break;
	case 0xF0: { // LDH A, (a8)
		LEN(2); CYCLE(12);
		uint16_t address = 0xFF00 + (uint16_t)memory_read(emu, emu->cpu.pc + 1);
		emu->cpu.a = memory_read(emu, address);
	} break;
	case 0xE2: { // LD (C), A
		LEN(1); CYCLE(8);
		uint16_t address = 0xFF00 + emu->cpu.c;
		memory_write(emu, address, emu->cpu.a);
	} break;
	case 0xF2: { // LD A, (C)
		LEN(1); CYCLE(8);
		uint16_t address = 0xFF00 + emu->cpu.c;
		emu->cpu.a = memory_read(emu, address);
	} break;
	case 0xEA: { // LD (a16), A
		LEN(3); CYCLE(16);
		uint16_t address = memory_read_16(emu, emu->cpu.pc + 1);
		memory_write(emu, address, emu->cpu.a);
	} break;
	case 0xFA: { // LD A, (a16)
		LEN(3); CYCLE(16);
		uint16_t address = memory_read_16(emu, emu->cpu.pc + 1);
		emu->cpu.a = memory_read(emu, address);
	} break;
	case 0x02: // LD (BC), A
		LEN(1); CYCLE(8);
		memory_write(emu, emu->cpu.bc, emu->cpu.a);
		break;
	case 0x12: // LD (DE), A
		LEN(1); CYCLE(8);
		memory_write(emu, emu->cpu.de, emu->cpu.a);
		break;
	case 0x0A: // LD A, (BC)
		LEN(1); CYCLE(8);
		emu->cpu.a = memory_read(emu, emu->cpu.bc);
		break;
	case 0x1A: // LD A, (DE)
		LEN(1); CYCLE(8);
		emu->cpu.a = memory_read(emu, emu->cpu.de);
		break;
	case 0x22: // LD (HL+), A
		LEN(1); CYCLE(8);
		memory_write(emu, emu->cpu.hl, emu->cpu.a);
		emu->cpu.hl++;
		break;
	case 0x32: // LD (HL-), A
		LEN(1); CYCLE(8);
		memory_write(emu, emu->cpu.hl, emu->cpu.a);
		emu->cpu.hl--;
		break;
	case 0x2A: // LD A, (HL+)
		LEN(1); CYCLE(8);
		emu->cpu.a = memory_read(emu, emu->cpu.hl);
		emu->cpu.hl++;
		break;
	case 0x3A: // LD A, (HL-)
		LEN(1); CYCLE(8);
		emu->cpu.a = memory_read(emu, emu->cpu.hl);
		emu->cpu.hl--;
		break;


	#define LDd8(TARGET) \
		LEN(2); CYCLE(8); emu->cpu.TARGET = memory_read(emu, emu->cpu.pc + 1); \
		break

	// LD r, d8
	case 0x06: LDd8(b);
	case 0x16: LDd8(d);
	case 0x26: LDd8(h);
	case 0x36: // LD (HL), d8;
		LEN(2); CYCLE(12);
		memory_write(emu, emu->cpu.hl, memory_read(emu, emu->cpu.pc + 1));
		break;
	case 0x0E: LDd8(c);
	case 0x1E: LDd8(e);
	case 0x2E: LDd8(l);
	case 0x3E: LDd8(a);


	#define LD(TARGET, SOURCE) \
		LEN(1); CYCLE(4); emu->cpu.TARGET = emu->cpu.SOURCE;\
		break

	#define LD_r_HL(TARGET) \
		LEN(1); CYCLE(8); emu->cpu.TARGET = memory_read(emu, emu->cpu.hl); \
		break

	#define LD_HL_r(SOURCE) \
		LEN(1); CYCLE(8); memory_write(emu, emu->cpu.hl, emu->cpu.SOURCE); \
		break
	
	// LD r,r
	case 0x40: LD(b, b);
	case 0x41: LD(b, c);
	case 0x42: LD(b, d);
	case 0x43: LD(b, e);
	case 0x44: LD(b, h);
	case 0x45: LD(b, l);
	case 0x46: LD_r_HL(b);
	case 0x47: LD(b, a);

	case 0x48: LD(c, b);
	case 0x49: LD(c, c);
	case 0x4A: LD(c, d);
	case 0x4B: LD(c, e);
	case 0x4C: LD(c, h);
	case 0x4D: LD(c, l);
	case 0x4E: LD_r_HL(c);
	case 0x4F: LD(c, a);

	case 0x50: LD(d, b);
	case 0x51: LD(d, c);
	case 0x52: LD(d, d);
	case 0x53: LD(d, e);
	case 0x54: LD(d, h);
	case 0x55: LD(d, l);
	case 0x56: LD_r_HL(d);
	case 0x57: LD(d, a);

	case 0x58: LD(e, b);
	case 0x59: LD(e, c);
	case 0x5A: LD(e, d);
	case 0x5B: LD(e, e);
	case 0x5C: LD(e, h);
	case 0x5D: LD(e, l);
	case 0x5E: LD_r_HL(e);
	case 0x5F: LD(e, a);

	case 0x60: LD(h, b);
	case 0x61: LD(h, c);
	case 0x62: LD(h, d);
	case 0x63: LD(h, e);
	case 0x64: LD(h, h);
	case 0x65: LD(h, l);
	case 0x66: LD_r_HL(h);
	case 0x67: LD(h, a);

	case 0x68: LD(l, b);
	case 0x69: LD(l, c);
	case 0x6A: LD(l, d);
	case 0x6B: LD(l, e);
	case 0x6C: LD(l, h);
	case 0x6D: LD(l, l);
	case 0x6E: LD_r_HL(l);
	case 0x6F: LD(l, a);

	case 0x70: LD_HL_r(b);
	case 0x71: LD_HL_r(c);
	case 0x72: LD_HL_r(d);
	case 0x73: LD_HL_r(e);
	case 0x74: LD_HL_r(h);
	case 0x75: LD_HL_r(l);
	case 0x77: LD_HL_r(a);

	case 0x78: LD(a, b);
	case 0x79: LD(a, c);
	case 0x7A: LD(a, d);
	case 0x7B: LD(a, e);
	case 0x7C: LD(a, h);
	case 0x7D: LD(a, l);
	case 0x7E: LD_r_HL(a);
	case 0x7F: LD(a, a);
	
	// ===========================
	// ========== ARITH ==========
	// ===========================

	#define exec_add(value) \
		uint8_t rhs = (value); \
		uint16_t result = (uint16_t)emu->cpu.a + (uint16_t)rhs; \
		bool hc = ((emu->cpu.a & 0xF) + (rhs & 0xF)) > 0xF; \
		SET_FLAG(result == 0, 0, hc, result > 0xFF); \
		emu->cpu.a = (uint8_t)result

	#define exec_adc(value) \
		uint8_t rhs = (value); \
		uint16_t result = (uint16_t)emu->cpu.a + (uint16_t)rhs + FLAG_C; \
		bool hc = ((emu->cpu.a & 0xF) + (rhs & 0xF) + FLAG_C) > 0xF; \
		SET_FLAG(result == 0, 0, hc, result > 0xFF); \
		emu->cpu.a = (uint8_t)result

	#define ADD(SOURCE) { LEN(1); CYCLE(4); exec_add(emu->cpu.SOURCE); } break
	#define ADC(SOURCE) { LEN(1); CYCLE(4); exec_adc(emu->cpu.SOURCE); } break

	case 0x80: ADD(b);
	case 0x81: ADD(c);
	case 0x82: ADD(d);
	case 0x83: ADD(e);
	case 0x84: ADD(h);
	case 0x85: ADD(l);
	case 0x86: { // ADD A, (HL)
		LEN(1); CYCLE(8);
		exec_add(memory_read(emu, emu->cpu.hl));
	} break;
	case 0x87: ADD(a);

	case 0x88: ADC(b);
	case 0x89: ADC(c);
	case 0x8A: ADC(d);
	case 0x8B: ADC(e);
	case 0x8C: ADC(h);
	case 0x8D: ADC(l);
	case 0x8E: { // ADC A, (HL)
		LEN(1); CYCLE(8);
		exec_adc(memory_read(emu, emu->cpu.hl));
	} break;
	case 0x8F: ADC(a);

	#define exec_sub(value) \
		uint8_t rhs = (value); \
		uint8_t result = emu->cpu.a - rhs; \
		bool hc = (emu->cpu.a & 0xF) < (rhs & 0xF); \
		SET_FLAG(result == 0, 1, hc, emu->cpu.a < rhs); \
		emu->cpu.a = result

	#define exec_sbc(value) \
		uint8_t rhs = (value); \
		uint8_t result = emu->cpu.a - rhs - FLAG_C; \
		bool hc = (emu->cpu.a & 0xF) < (rhs & 0xF) + FLAG_C; \
		SET_FLAG(result == 0, 1, hc, (uint16_t)emu->cpu.a < (uint16_t)rhs + FLAG_C); \
		emu->cpu.a = result

	#define SUB(SOURCE) { LEN(1); CYCLE(4); exec_sub(emu->cpu.SOURCE); } break
	#define SBC(SOURCE) { LEN(1); CYCLE(4); exec_sbc(emu->cpu.SOURCE); } break

	case 0x90: SUB(b);
	case 0x91: SUB(c);
	case 0x92: SUB(d);
	case 0x93: SUB(e);
	case 0x94: SUB(h);
	case 0x95: SUB(l);
	case 0x96: { // SUB (HL)
		LEN(1); CYCLE(8);
		exec_sub(memory_read(emu, emu->cpu.hl));
	} break;
	case 0x97: SUB(a);

	case 0x98: SBC(b);
	case 0x99: SBC(c);
	case 0x9A: SBC(d);
	case 0x9B: SBC(e);
	case 0x9C: SBC(h);
	case 0x9D: SBC(l);
	case 0x9E: { // SBC (HL)
		LEN(1); CYCLE(8);
		exec_sbc(memory_read(emu, emu->cpu.hl));
	} break;
	case 0x9F: SBC(a);

	#define exec_and(value) \
		uint8_t rhs = (value); \
		emu->cpu.a = emu->cpu.a & rhs; \
		SET_FLAG(emu->cpu.a == 0, 0, 1, 0)

	#define exec_xor(value) \
		uint8_t rhs = (value); \
		emu->cpu.a = emu->cpu.a ^ rhs; \
		SET_FLAG(emu->cpu.a == 0, 0, 0, 0)

	#define exec_or(value) \
		uint8_t rhs = (value); \
		emu->cpu.a = emu->cpu.a | rhs; \
		SET_FLAG(emu->cpu.a == 0, 0, 0, 0)
	
	#define AND(SOURCE) { LEN(1); CYCLE(4); exec_and(emu->cpu.SOURCE); } break
	#define XOR(SOURCE) { LEN(1); CYCLE(4); exec_xor(emu->cpu.SOURCE); } break
	#define OR(SOURCE) { LEN(1); CYCLE(4); exec_or(emu->cpu.SOURCE); } break
	
	#define exec_cp(value) \
		uint8_t rhs = (value); \
		uint8_t result = emu->cpu.a - rhs; \
		bool hc = (emu->cpu.a & 0xF) < (rhs & 0xF); \
		SET_FLAG(result == 0, 1, hc, emu->cpu.a < rhs)

	#define CP(SOURCE) { LEN(1); CYCLE(4); exec_cp(emu->cpu.SOURCE); } break

	case 0xA0: AND(b);
	case 0xA1: AND(c);
	case 0xA2: AND(d);
	case 0xA3: AND(e);
	case 0xA4: AND(h);
	case 0xA5: AND(l);
	case 0xA6: { // AND (HL)
		LEN(1); CYCLE(8);
		exec_and(memory_read(emu, emu->cpu.hl));
	} break;
	case 0xA7: AND(a);

	case 0xA8: XOR(b);
	case 0xA9: XOR(c);
	case 0xAA: XOR(d);
	case 0xAB: XOR(e);
	case 0xAC: XOR(h);
	case 0xAD: XOR(l);
	case 0xAE: { // XOR (HL)
		LEN(1); CYCLE(8);
		exec_xor(memory_read(emu, emu->cpu.hl));
	} break;
	case 0xAF: XOR(a);

	case 0xB0: OR(b);
	case 0xB1: OR(c);
	case 0xB2: OR(d);
	case 0xB3: OR(e);
	case 0xB4: OR(h);
	case 0xB5: OR(l);
	case 0xB6: { // OR (HL)
		LEN(1); CYCLE(8);
		exec_or(memory_read(emu, emu->cpu.hl));
	} break;
	case 0xB7: OR(a);

	case 0xB8: CP(b);
	case 0xB9: CP(c);
	case 0xBA: CP(d);
	case 0xBB: CP(e);
	case 0xBC: CP(h);
	case 0xBD: CP(l);
	case 0xBE: { // CP (HL)
		LEN(1); CYCLE(8);
		exec_cp(memory_read(emu, emu->cpu.hl));
	} break;
	case 0xBF: CP(a);
	
	case 0xC6: { // ADD A, d8
		LEN(2); CYCLE(8);
		exec_add(memory_read(emu, emu->cpu.pc + 1)); 
	} break;
	case 0xD6: { // SUB A, d8
		LEN(2); CYCLE(8); exec_sub(memory_read(emu, emu->cpu.pc + 1));
	} break;
	case 0xE6: { // AND A, d8
		LEN(2); CYCLE(8); exec_and(memory_read(emu, emu->cpu.pc + 1));
	} break;
	case 0xF6: { // OR A, d8
		LEN(2); CYCLE(8); exec_or(memory_read(emu, emu->cpu.pc + 1));
	} break;

	case 0xCE: { // ADC A, d8
		LEN(2); CYCLE(8); exec_adc(memory_read(emu, emu->cpu.pc + 1));
	} break;
	case 0xDE: { // SBC A, d8
		LEN(2); CYCLE(8); exec_sbc(memory_read(emu, emu->cpu.pc + 1));
	} break;
	case 0xEE: { // XOR A, d8
		LEN(2); CYCLE(8); exec_xor(memory_read(emu, emu->cpu.pc + 1));
	} break;
	case 0xFE: { // CP A, d8
		LEN(2); CYCLE(8); exec_cp(memory_read(emu, emu->cpu.pc + 1));
	} break;
	
	case 0x27: { // DAA
		LEN(1); CYCLE(4);
		bool carry = FLAG_C;
		uint8_t a = emu->cpu.a;
		if (FLAG_N) {
			if (FLAG_C) { a -= 0x60; }
			if (FLAG_H) { a -= 0x06; }
		} else {
			if (FLAG_C || a > 0x99) { a += 0x60; carry = true; }
			if (FLAG_H || (a & 0x0F) > 0x09) { a += 0x06; }
		}
		emu->cpu.a = a;
		SET_FLAG(emu->cpu.a == 0, FLAG_N, 0, carry);
	} break;
	case 0x37: // SCF
		LEN(1); CYCLE(4);
		SET_FLAG(FLAG_Z, 0, 0, 1);
		break;
	case 0x2F: // CPL
		LEN(1); CYCLE(4); 
		emu->cpu.a ^= 0xFF; 
		SET_FLAG(FLAG_Z, 1, 1, FLAG_C);
		break;
	case 0x3F: // CCF
		LEN(1); CYCLE(4); 
		SET_FLAG(FLAG_Z, 0, 0, !FLAG_C); 
		break;

	#define exec_inc(value) \
		uint8_t rhs = (value); \
		uint16_t result = (uint16_t)rhs + 1; \
		bool hc = (rhs & 0xF) == 0xF; \
		SET_FLAG(result == 0, 0, hc, FLAG_C); \
	

	#define exec_dec(value) \
		uint8_t rhs = (value); \
		uint8_t result = rhs - 1; \
		bool hc = (rhs & 0xF) == 0x00; \
		SET_FLAG(result == 0, 1, hc, FLAG_C); \

	#define INC(SOURCE) { \
		LEN(1); CYCLE(4); exec_inc(emu->cpu.SOURCE); \
		emu->cpu.SOURCE = (uint8_t)result; \
	} break
	#define DEC(SOURCE) { \
		LEN(1); CYCLE(4); exec_dec(emu->cpu.SOURCE); \
		emu->cpu.SOURCE = result; \
	} break

	case 0x04: INC(b);
	case 0x14: INC(d);
	case 0x24: INC(h);
	case 0x34: {
		LEN(1); CYCLE(12);
		exec_inc(memory_read(emu, emu->cpu.hl));
		memory_write(emu, emu->cpu.hl, result);
	} break;
	case 0x0C: INC(c);
	case 0x1C: INC(e);
	case 0x2C: INC(l);
	case 0x3C: INC(a);

	case 0x05: DEC(b);
	case 0x15: DEC(d);
	case 0x25: DEC(h);
	case 0x35: {
		LEN(1); CYCLE(12);
		exec_dec(memory_read(emu, emu->cpu.hl));
		memory_write(emu, emu->cpu.hl, result);
	} break;
	case 0x0D: DEC(c);
	case 0x1D: DEC(e);
	case 0x2D: DEC(l);
	case 0x3D: DEC(a);


	// =============================
	// ========== ARITH16 ==========
	// =============================


	#define INC16(TARGET) LEN(1); CYCLE(8); emu->cpu.TARGET += 1; break;
	#define DEC16(TARGET) LEN(1); CYCLE(8); emu->cpu.TARGET -= 1; break;
	#define ADD16(TARGET) { \
		LEN(1); CYCLE(8); \
		uint16_t rhs = (emu->cpu.TARGET); \
		uint32_t result = (uint32_t)emu->cpu.hl + (uint32_t)rhs; \
		bool hc = ((emu->cpu.hl & 0x0FFF) + (rhs & 0x0FFF)) > 0x0FFF; \
		SET_FLAG(FLAG_Z, 0, hc, result > 0xFFFF); \
		emu->cpu.hl = (uint16_t)result; \
	} break

	case 0x03: INC16(bc);
	case 0x13: INC16(de);
	case 0x23: INC16(hl);
	case 0x33: INC16(sp);
	case 0x0B: DEC16(bc);
	case 0x1B: DEC16(de);
	case 0x2B: DEC16(hl);
	case 0x3B: DEC16(sp);

	case 0x09: ADD16(bc);
	case 0x19: ADD16(de);
	case 0x29: ADD16(hl);
	case 0x39: ADD16(sp);

	case 0xE8: { // SP, r8;
		LEN(2); CYCLE(16);
		int8_t offset = (int8_t)memory_read(emu, emu->cpu.pc + 1);
		uint8_t uoffset = (uint8_t)offset;
		uint8_t sp_low = emu->cpu.sp & 0xFF;
		bool hc = (sp_low & 0xF) + (uoffset & 0xF) > 0xF;
		bool c = (sp_low + uoffset) > 0xFF;
		SET_FLAG(0, 0, hc, c);
		emu->cpu.sp = emu->cpu.sp + offset;
	} break;


	// =============================
	// ========== CONTROL ==========
	// =============================
	
	#define JR(flag) { \
		LEN(2); \
		if (flag) { \
			CYCLE(12); \
			int8_t offset = (int8_t)memory_read(emu, emu->cpu.pc + 1); \
			emu->cpu.pc += offset; \
		}  else { CYCLE(8); } \
	} break

	case 0x20: JR(!FLAG_Z); // JR NZ, r8
	case 0x30: JR(!FLAG_C); // JR NC, r8
	case 0x18: JR(true); // JR r8
	case 0x28: JR(FLAG_Z); // JR Z, r8
	case 0x38: JR(FLAG_C); // JR C, r8
	
	#define do_return() \
		emu->cpu.pc = memory_read_16(emu, emu->cpu.sp); \
		emu->cpu.sp += 2

	#define RET(flag) { \
		if (flag) { LEN(0); CYCLE(20); do_return(); } \
		else      { LEN(1); CYCLE(8); } \
	} break
	case 0xC0: RET(!FLAG_Z); // RET NZ
	case 0xD0: RET(!FLAG_C); // RET NC
	case 0xC8: RET(FLAG_Z); // RET Z
	case 0xD8: RET(FLAG_C); // RET C
	case 0xC9: { LEN(0); CYCLE(8); do_return(); } break; // RET
	case 0xD9: { // RETI
		LEN(0); CYCLE(16);
		do_return();
		emu->interrupt.ime = true;
	} break;
	
	#define do_jump(target) emu->cpu.pc = (target)
	#define JP(flag) { \
		if (flag) { CYCLE(16); LEN(0); do_jump(memory_read_16(emu, emu->cpu.pc + 1)); } \
			else {CYCLE(12); LEN(3); } \
	} break

	case 0xC2: JP(!FLAG_Z); // JP NZ, a16
	case 0xD2: JP(!FLAG_C); // JP NC, a16
	case 0xCA: JP(FLAG_Z); // JP Z, a16
	case 0xDA: JP(FLAG_C); // JP C, a16
	case 0xC3: JP(true);
	case 0xE9: {
		LEN(0); CYCLE(4); do_jump(emu->cpu.hl);
	} break; // JP (HL)

	#define do_call() \
		emu->cpu.sp -= 2; \
		memory_write_16(emu, emu->cpu.sp, emu->cpu.pc + 3); \
		emu->cpu.pc = memory_read_16(emu, emu->cpu.pc + 1)

	#define CALL(flag) { \
		if (flag) { CYCLE(24); LEN(0); do_call(); \
		} else { CYCLE(12); LEN(3); } \
	} break

	case 0xC4: CALL(!FLAG_Z); // CALL NZ, a16
	case 0xD4: CALL(!FLAG_C); // CALL NC, a16
	case 0xCC: CALL(FLAG_Z); // CALL Z, a16
	case 0xDC: CALL(FLAG_C); // CALL C, a16
	case 0xCD: CALL(true); // CALL a16
	
	#define RST(target) { \
		LEN(0); CYCLE(16); \
		emu->cpu.sp -= 2; \
		memory_write_16(emu, emu->cpu.sp, emu->cpu.pc + 1); \
		emu->cpu.pc = target; \
	} break
	
	case 0xC7: RST(0x00); // RST 00H
	case 0xD7: RST(0x10); // RST 10H
	case 0xE7: RST(0x20); // RST 20H
	case 0xF7: RST(0x30); // RST 30H
	case 0xCF: RST(0x08); // RST 08H
	case 0xDF: RST(0x18); // RST 18H
	case 0xEF: RST(0x28); // RST 28H
	case 0xFF: RST(0x38); // RST 38H
	
	// ============================
	// ========== ROTATE ==========
	// ============================
	
	case 0x07: { // RLCA
		LEN(1); CYCLE(4);
		uint8_t bit = emu->cpu.a >> 7;
		emu->cpu.a = (emu->cpu.a << 1) | bit;
		SET_FLAG(0, 0, 0, bit);
	} break;
	case 0x17: { // RLA
		LEN(1); CYCLE(4);
		uint8_t bit = emu->cpu.a >> 7;
		emu->cpu.a = (emu->cpu.a << 1) | FLAG_C;
		SET_FLAG(0, 0, 0, bit);
	} break;
	case 0x0F: { // RRCA
		LEN(1); CYCLE(4);
		uint8_t bit = emu->cpu.a & 0b1;
		emu->cpu.a = (emu->cpu.a >> 1) | (bit << 7);
		SET_FLAG(0, 0, 0, bit);
	} break;
	case 0x1F: { // RRA
		LEN(1); CYCLE(4);
		uint8_t bit = emu->cpu.a & 0b1;
		emu->cpu.a = (emu->cpu.a >> 1) | (FLAG_C << 7);
		SET_FLAG(0, 0, 0, bit);
	} break;


	#define NO_INSTRUCTION LEN(1); CYCLE(4); DEBUG("%02X is an invalid opcode!", opcode); break
	case 0xD3: NO_INSTRUCTION;
	case 0xE3: NO_INSTRUCTION;
	case 0xE4: NO_INSTRUCTION;
	case 0xF4: NO_INSTRUCTION;
	case 0xDB: NO_INSTRUCTION;
	case 0xEB: NO_INSTRUCTION;
	case 0xEC: NO_INSTRUCTION;
	case 0xED: NO_INSTRUCTION;
	case 0xFC: NO_INSTRUCTION;
	case 0xFD: NO_INSTRUCTION;
	case 0xDD: NO_INSTRUCTION;
	

	default:
		DEBUG("Not implemented opcode %02X", opcode);
	}
}

#define HL_OP(METHOD) { \
	LEN(2); CYCLE(16); \
	uint8_t value = memory_read(emu, emu->cpu.hl); \
	METHOD(); \
	SET_FLAG(result == 0, 0, 0, bit); \
	memory_write(emu, emu->cpu.hl, result); \
} break
#define BIT_OP(TARGET, METHOD) { \
	LEN(2); CYCLE(8); \
	uint8_t value = emu->cpu.TARGET; \
	METHOD() \
	SET_FLAG(result == 0, 0, 0, bit); \
	emu->cpu.TARGET = result; \
} break
static inline void prefix_opcodes(Emulator* emu) {
	uint8_t opcode = memory_read(emu, emu->cpu.pc + 1);
	switch (opcode) {
	
	#define do_rlc() \
		uint8_t bit = value >> 7; \
		uint8_t result = (value << 1) | bit;
	#define do_rl() \
		uint8_t bit = value >> 7; \
		uint8_t result = (value << 1) | FLAG_C;
	#define do_rrc() \
		uint8_t bit = value & 0b1; \
		uint8_t result = (value >> 1) | (bit << 7);
	#define do_rr() \
		uint8_t bit = value & 0b1; \
		uint8_t result = (value >> 1) | (FLAG_C << 7);
	
	#define RLC(TARGET) BIT_OP(TARGET, do_rlc)
	#define RL(TARGET) BIT_OP(TARGET, do_rl)
	#define RRC(TARGET) BIT_OP(TARGET, do_rrc)
	#define RR(TARGET) BIT_OP(TARGET, do_rr)

	case 0x00: RLC(b);
	case 0x01: RLC(c);
	case 0x02: RLC(d);
	case 0x03: RLC(e);
	case 0x04: RLC(h);
	case 0x05: RLC(l);
	case 0x07: RLC(a);
	case 0x06: HL_OP(do_rlc);

	case 0x08: RRC(b);
	case 0x09: RRC(c);
	case 0x0A: RRC(d);
	case 0x0B: RRC(e);
	case 0x0C: RRC(h);
	case 0x0D: RRC(l);
	case 0x0F: RRC(a);
	case 0x0E: HL_OP(do_rrc);
	
	case 0x10: RL(b);
	case 0x11: RL(c);
	case 0x12: RL(d);
	case 0x13: RL(e);
	case 0x14: RL(h);
	case 0x15: RL(l);
	case 0x17: RL(a);
	case 0x16: HL_OP(do_rl);

	case 0x18: RR(b);
	case 0x19: RR(c);
	case 0x1A: RR(d);
	case 0x1B: RR(e);
	case 0x1C: RR(h);
	case 0x1D: RR(l);
	case 0x1F: RR(a);
	case 0x1E: HL_OP(do_rr);

	#define do_sla() \
		uint8_t bit = value >> 7; \
		uint8_t result = (value << 1);
	// Shift n right into carry, MSB does not change
	#define do_sra() \
		uint8_t bit = value & 0b1; \
		uint8_t result = (value >> 1) | (value & 0b10000000);
	// Shift n right into carry, MSB set to 0
	#define do_srl() \
		uint8_t bit = value & 0b1; \
		uint8_t result = (value >> 1);
	#define do_swap() \
		uint8_t bit = 0; \
		uint8_t result = (value << 4) | (value >> 4);
	
	#define SLA(TARGET) BIT_OP(TARGET, do_sla)
	#define SRA(TARGET) BIT_OP(TARGET, do_sra)
	#define SRL(TARGET) BIT_OP(TARGET, do_srl)
	#define SWAP(TARGET) BIT_OP(TARGET, do_swap)

	case 0x20: SLA(b);
	case 0x21: SLA(c);
	case 0x22: SLA(d);
	case 0x23: SLA(e);
	case 0x24: SLA(h);
	case 0x25: SLA(l);
	case 0x27: SLA(a);
	case 0x26: HL_OP(do_sla);
	case 0x28: SRA(b);
	case 0x29: SRA(c);
	case 0x2A: SRA(d);
	case 0x2B: SRA(e);
	case 0x2C: SRA(h);
	case 0x2D: SRA(l);
	case 0x2F: SRA(a);
	case 0x2E: HL_OP(do_sra);

	case 0x30: SWAP(b);
	case 0x31: SWAP(c);
	case 0x32: SWAP(d);
	case 0x33: SWAP(e);
	case 0x34: SWAP(h);
	case 0x35: SWAP(l);
	case 0x37: SWAP(a);
	case 0x36: HL_OP(do_swap);
	case 0x38: SRL(b);
	case 0x39: SRL(c);
	case 0x3A: SRL(d);
	case 0x3B: SRL(e);
	case 0x3C: SRL(h);
	case 0x3D: SRL(l);
	case 0x3F: SRL(a);
	case 0x3E: HL_OP(do_srl);
	

	#define LINE(START, OFFSET, REG_METHOD, ADDR_METHOD) \
		case START + 0: REG_METHOD(OFFSET, b); \
		case START + 1: REG_METHOD(OFFSET, c); \
		case START + 2: REG_METHOD(OFFSET, d); \
		case START + 3: REG_METHOD(OFFSET, e); \
		case START + 4: REG_METHOD(OFFSET, h); \
		case START + 5: REG_METHOD(OFFSET, l); \
		case START + 6: ADDR_METHOD(OFFSET); \
		case START + 7: REG_METHOD(OFFSET, a)


	#define BIT(OFFSET, TARGET) { \
		LEN(2); CYCLE(8); \
		uint8_t bit = (emu->cpu.TARGET >> OFFSET) & 0b1; \
		SET_FLAG(!bit, 0, 1, FLAG_C); \
	} break
	#define BIT_HL(OFFSET) { \
		LEN(2); CYCLE(12); \
		uint8_t bit = (memory_read(emu, emu->cpu.hl) >> OFFSET) & 0b1; \
		SET_FLAG(!bit, 0, 1, FLAG_C); \
	} break

	#define BITS(START, OFFSET) LINE(START, OFFSET, BIT, BIT_HL)
	BITS(0x40, 0);
	BITS(0x48, 1);
	BITS(0x50, 2);
	BITS(0x58, 3);
	BITS(0x60, 4);
	BITS(0x68, 5);
	BITS(0x70, 6);
	BITS(0x78, 7);

	#define RES(OFFSET, TARGET) \
		LEN(2); CYCLE(8); \
		emu->cpu.TARGET &= ~(1 << OFFSET); \
		break
	
	#define RES_HL(OFFSET) { \
		LEN(2); CYCLE(16); \
		uint8_t value = memory_read(emu, emu->cpu.hl); \
		memory_write(emu, emu->cpu.hl, value & ~(1 << OFFSET)); \
	} break

	#define SET(OFFSET, TARGET) \
		LEN(2); CYCLE(8); \
		emu->cpu.TARGET |= (1 << OFFSET); \
		break
	
	#define SET_HL(OFFSET) { \
		LEN(2); CYCLE(16); \
		uint8_t value = memory_read(emu, emu->cpu.hl); \
		memory_write(emu, emu->cpu.hl, value | (1 << OFFSET)); \
	} break
	
	#define RESS(START, OFFSET) LINE(START, OFFSET, RES, RES_HL)
	#define SETS(START, OFFSET) LINE(START, OFFSET, SET, SET_HL)
	
	RESS(0x80, 0);
	RESS(0x88, 1);
	RESS(0x90, 2);
	RESS(0x98, 3);
	RESS(0xA0, 4);
	RESS(0xA8, 5);
	RESS(0xB0, 6);
	RESS(0xB8, 7);

	SETS(0xC0, 0);
	SETS(0xC8, 1);
	SETS(0xD0, 2);
	SETS(0xD8, 3);
	SETS(0xE0, 4);
	SETS(0xE8, 5);
	SETS(0xF0, 6);
	SETS(0xF8, 7);

	default:
		DEBUG("Not implemented prefix opcode %x", opcode);
	}
}
