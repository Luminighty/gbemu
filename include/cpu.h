#ifndef CPU_H
#define CPU_H

#include <stdint.h>


#define REGISTER_PAIR(HI, LO, MERGED) \
	union { struct { uint8_t LO; uint8_t HI; }; uint16_t MERGED; }

typedef struct {
	REGISTER_PAIR(a, f, af);
	REGISTER_PAIR(b, c, bc);
	REGISTER_PAIR(d, e, de);
	REGISTER_PAIR(h, l, hl);

	uint16_t pc;
	uint16_t sp;

	uint8_t opcode_length;
	uint8_t cycles;
} CPU;


CPU cpu_create();


#endif // CPU_H
