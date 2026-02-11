#include "memory_map.h"

#include "interrupts.h"
#include "logger.h"
#include "timer.h"

uint16_t memory_read_16(Emulator *emu, uint16_t address) {
	return memory_read(emu, address) | memory_read(emu, address + 1) << 8;
}

void memory_write_16(Emulator *emu, uint16_t address, uint16_t value) {
	memory_write(emu, address, value & 0xFF);
	memory_write(emu, address + 1, value >> 8);
}


uint8_t memory_read(Emulator *emu, uint16_t address) {
	// DEBUG("READ %04X\n", address);
	// TODO: Will require a mapper!
	if (address <= 0x7FFF)
		return emu->cartridge->content[address];

	// NOTE: VRAM
	if (0x8000 <= address && address <= 0x9FFF)
		return emu->memory->temp_vram[address - 0x8000];

	
	// NOTE: SWITCH WRAM

	// NOTE: WRAM
	if (address >= 0xC000 && address <= 0xDFFF)
		return emu->memory->wram[address - 0xC000];

	// NOTE: Echo of WRAM
	if (address >= 0xE000 && address <= 0xFDFF)
		return memory_read(emu, address - 0x2000);
	
	// NOTE: OAM
	
	// NOTE: Empty IO
	
	// NOTE: IO PORTS
	
	switch (address) {
	// NOTE: Timers
	case 0xFF04: return timer_div_read(emu);
	case 0xFF05: return emu->timer.tima;
	case 0xFF06: return emu->timer.tma;
	case 0xFF07: return timer_tac_read(emu);
	// NOTE: PPU
	case 0xFF44: return emu->ppu.line;
	case 0xFF40: return emu->ppu.lcdc;
	case 0xFF41: return emu->ppu.stat;
	case 0xFF47: return emu->ppu.bgp;
	// NOTE: Interrupts
	case 0xFF0F: return interrupt_flag_read(&emu->interrupt);
	case 0xFFFF: return interrupt_enable_read(&emu->interrupt);
	}

	// NOTE: High WRAM
	if (address >= 0xFF80 && address <= 0xFFFE)
		return emu->memory->highram[address - 0xFF80];


	DEBUG("Reading unmapped address: %x", address);

	return 0xFF;
}

void memory_write(Emulator *emu, uint16_t address, uint8_t value) {
	// TODO: Will require a mapper!
	if (address <= 0x7FFF) {
		// TODO: Bank switching writes
		return;
	}

	// NOTE: VRAM
	if (0x8000 <= address && address <= 0x9FFF) {
		// printf("[VRAM WRITE] %04X = %02X\n", address, value);
		emu->memory->temp_vram[address - 0x8000] = value;
		return;
	}
	
	// NOTE: SWITCH WRAM

	// NOTE: WRAM
	if (address >= 0xC000 && address <= 0xDFFF) {
		emu->memory->wram[address - 0xC000] = value;
		return;
	}

	// NOTE: Echo of WRAM
	if (address >= 0xE000 && address <= 0xFDFF)
		return memory_write(emu, address - 0x2000, value);
	
	// NOTE: OAM
	
	// NOTE: Empty IO
	
	// NOTE: IO PORTS
	switch (address) {
	case 0xFF04: timer_div_reset(emu); return;
	case 0xFF05: timer_tima_write(emu, value); return;
	case 0xFF06: timer_tma_write(emu, value); return;
	case 0xFF07: timer_tac_write(emu, value); return;
	// NOTE: PPU
	// TODO: Update these
	case 0xFF44: emu->ppu.line = value; return;
	case 0xFF40: emu->ppu.lcdc = value; return;
	case 0xFF41: emu->ppu.stat = value; return;
	case 0xFF47: emu->ppu.bgp = value; return;
	// NOTE: Interrupts
	case 0xFF0F: interrupt_flag_write(&emu->interrupt, value); return;
	case 0xFFFF: interrupt_enable_write(&emu->interrupt, value); return;
	}
	
	// NOTE: High WRAM
	if (address >= 0xFF80 && address <= 0xFFFE) {
		emu->memory->highram[address - 0xFF80] = value;
		return;
	}

	// NOTE: Interrupt Enable
	if (address == 0xFFFF) {
		emu->memory->interrupt_enabled = value;
		return;
	}

	DEBUG("Writing to unmapped address: [%x] = %x", address, value);
}

