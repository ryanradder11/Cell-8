#ifndef CHIP8_H
#define CHIP8_H

#include <stdint.h>

// Ported as-is from ~/projects/chip8-emulator/main.cpp (a standalone
// SDL2/desktop CHIP-8 interpreter). Only the platform glue around it
// changed -- struct layout and opcode interpreter logic are unchanged,
// including its existing bugs/gaps (kept intentionally, not fixed here):
//   - several 0x8000 sub-opcodes (0x3, 0x5, 0x6, 0x7) are missing a
//     `break` and fall through into the next case
//   - no built-in hex-digit font is loaded into memory, so Fx29
//     (set I to a font sprite address) won't point at real glyph data
//   - BNNN (0xB000) has an operator-precedence bug: `opcode & 0x0FFF + chip.V[0]`
//     adds V0 to the immediate before masking, not after
// std::cout/std::ifstream were swapped for printf/fopen (same debug
// output, same file-loading behavior) since this toolchain's libstdc++
// iostream support is untested here and this project uses printf
// everywhere else.

extern bool debug;

struct Chip8 {
	bool drawFlag; // Set to true if the screen needs to be redrawn
	uint8_t memory[4096];
	uint8_t V[16];           // Registers V0 to VF
	uint16_t I;              // Index register
	uint16_t pc;             // Program counter
	uint8_t gfx[64 * 32];    // Display (monochrome 64x32)
	uint8_t delay_timer;
	uint8_t sound_timer;
	uint16_t stack[16];
	uint16_t sp;             // Stack pointer
	uint8_t keypad[16];      // Hex-based keypad (0x0-0xF)
};

bool loadROM(const char *filename, Chip8 &chip);
void emulateCycle(Chip8 &chip);

#endif // CHIP8_H
