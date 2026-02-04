#include "cpu.h"


CPU cpu_create() {
	CPU cpu = {0};
	cpu.pc = 0x0100;
	return cpu;
}

