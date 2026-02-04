#include "opcode.h"
#include "logger.h"
#include "memory_map.h"
#include <stdint.h>

#define LEN(c) emu->cpu.opcode_length = c
#define CYCLE(c) emu->cpu.cycles = c

#define MASK_FLAG emu->cpu.f &= 0xF0

#define SET_FLAG(Z, N, H, C) emu->cpu.f = ((Z) << 7) | ((N) << 6) | ((H) << 5) | ((C) << 4)

#define FLAG_C ((emu->cpu.f >> 4) & 1)
#define FLAG_H ((emu->cpu.f >> 5) & 1)
#define FLAG_N ((emu->cpu.f >> 6) & 1)
#define FLAG_Z ((emu->cpu.f >> 7) & 1)

void opcode_execute(Emulator* emu, uint8_t opcode) {
	switch (opcode) {
	// NOOP
	case 0x00: LEN(1); CYCLE(4); break;

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
	// TODO: Random LD cases
	case 0xF8: break; // LD HL, SP+r8
	case 0xF9: break; // LD SP, HL
	
	// ========================
	// ========== LD ==========
	// ========================

	// TODO: Random LD cases
	case 0xE0: break; // LDH (a8), A
	case 0xF0: break; // LDH A, (a8)
	case 0xE2: break; // LD (C), A
	case 0xF2: break; // LD A, (C)
	case 0xEA: break; // LD (a16), A
	case 0xFA: break; // LD A, (a16)

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
		LEN(2); CYCLE(12); memory_write(emu, emu->cpu.hl, memory_read(emu, emu->cpu.pc + 1));
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
	// case 0x76: HALT
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
		LEN(2); CYCLE(8); exec_add(memory_read(emu, emu->cpu.pc + 1)); 
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
	
	// TODO: DAA
	case 0x27: break;
	// TODO: SCF
	case 0x37: break;
	// TODO: CPL
	case 0x2F: break;
	// TODO: CCF
	case 0x3F: break;

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


	case 0x03: INC16(bc);
	case 0x13: INC16(de);
	case 0x23: INC16(hl);
	case 0x33: INC16(sp);
	case 0x0B: DEC16(bc);
	case 0x1B: DEC16(de);
	case 0x2B: DEC16(hl);
	case 0x3B: DEC16(sp);

	// TODO: ADD16
	// case 0x09: ADD16(bc);
	// case 0x19: ADD16(de);
	// case 0x29: ADD16(hl);
	// case 0x39: ADD16(sp);

	// TODO: ADD SP, r8
	// case 0xE8: SP, r8;

	default:
		DEBUG("Not implemented opcode %x", opcode);
	}
}

