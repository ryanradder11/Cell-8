#include "font5x7.h"

#define FONT_GLYPH_W 5
#define FONT_GLYPH_H 7
#define GLYPH_SPACING 1 // blank columns between glyphs

// Each row is 5 bits, bit 4 = leftmost pixel .. bit 0 = rightmost pixel.
typedef u8 Glyph[FONT_GLYPH_H];

static const Glyph GLYPH_SPACE = {
	0b00000,
	0b00000,
	0b00000,
	0b00000,
	0b00000,
	0b00000,
	0b00000,
};

static const Glyph GLYPH_MINUS = {
	0b00000,
	0b00000,
	0b00000,
	0b11111,
	0b00000,
	0b00000,
	0b00000,
};

static const Glyph GLYPH_A = {
	0b01110,
	0b10001,
	0b10001,
	0b11111,
	0b10001,
	0b10001,
	0b10001,
};

static const Glyph GLYPH_C = {
	0b01110,
	0b10000,
	0b10000,
	0b10000,
	0b10000,
	0b10000,
	0b01110,
};

static const Glyph GLYPH_E = {
	0b11111,
	0b10000,
	0b10000,
	0b11110,
	0b10000,
	0b10000,
	0b11111,
};

static const Glyph GLYPH_H = {
	0b10001,
	0b10001,
	0b10001,
	0b11111,
	0b10001,
	0b10001,
	0b10001,
};

static const Glyph GLYPH_I = {
	0b01110,
	0b00100,
	0b00100,
	0b00100,
	0b00100,
	0b00100,
	0b01110,
};

static const Glyph GLYPH_L = {
	0b10000,
	0b10000,
	0b10000,
	0b10000,
	0b10000,
	0b10000,
	0b11111,
};

static const Glyph GLYPH_M = {
	0b10001,
	0b11011,
	0b10101,
	0b10101,
	0b10001,
	0b10001,
	0b10001,
};

static const Glyph GLYPH_O = {
	0b01110,
	0b10001,
	0b10001,
	0b10001,
	0b10001,
	0b10001,
	0b01110,
};

static const Glyph GLYPH_P = {
	0b11110,
	0b10001,
	0b10001,
	0b11110,
	0b10000,
	0b10000,
	0b10000,
};

static const Glyph GLYPH_R = {
	0b11110,
	0b10001,
	0b10001,
	0b11110,
	0b10100,
	0b10010,
	0b10001,
};

static const Glyph GLYPH_S = {
	0b01111,
	0b10000,
	0b10000,
	0b01110,
	0b00001,
	0b00001,
	0b11110,
};

static const Glyph GLYPH_T = {
	0b11111,
	0b00100,
	0b00100,
	0b00100,
	0b00100,
	0b00100,
	0b00100,
};

static const Glyph GLYPH_U = {
	0b10001,
	0b10001,
	0b10001,
	0b10001,
	0b10001,
	0b10001,
	0b01110,
};

static const Glyph GLYPH_X = {
	0b10001,
	0b10001,
	0b01010,
	0b00100,
	0b01010,
	0b10001,
	0b10001,
};

static const Glyph GLYPH_0 = {
	0b01110,
	0b10011,
	0b10101,
	0b10101,
	0b10101,
	0b11001,
	0b01110,
};

static const Glyph GLYPH_8 = {
	0b01110,
	0b10001,
	0b10001,
	0b01110,
	0b10001,
	0b10001,
	0b01110,
};

static const Glyph *getGlyph(char c)
{
	switch (c) {
		case 'A': return &GLYPH_A;
		case 'C': return &GLYPH_C;
		case 'E': return &GLYPH_E;
		case 'H': return &GLYPH_H;
		case 'I': return &GLYPH_I;
		case 'L': return &GLYPH_L;
		case 'M': return &GLYPH_M;
		case 'O': return &GLYPH_O;
		case 'P': return &GLYPH_P;
		case 'R': return &GLYPH_R;
		case 'S': return &GLYPH_S;
		case 'T': return &GLYPH_T;
		case 'U': return &GLYPH_U;
		case 'X': return &GLYPH_X;
		case '0': return &GLYPH_0;
		case '8': return &GLYPH_8;
		case '-': return &GLYPH_MINUS;
		default:  return &GLYPH_SPACE; // includes ' ' and anything unmapped
	}
}

static inline void fillBlock(u32 *buffer, u32 pitchPixels, s32 x, s32 y, u32 scale, u32 color)
{
	for (u32 dy = 0; dy < scale; dy++) {
		for (u32 dx = 0; dx < scale; dx++) {
			buffer[(y + dy) * pitchPixels + (x + dx)] = color;
		}
	}
}

void drawText5x7(u32 *buffer, u32 pitchPixels, s32 x, s32 y, const char *text, u32 color, u32 scale)
{
	s32 penX = x;

	for (const char *p = text; *p; p++) {
		const Glyph *glyph = getGlyph(*p);

		for (s32 row = 0; row < FONT_GLYPH_H; row++) {
			u8 bits = (*glyph)[row];
			for (s32 col = 0; col < FONT_GLYPH_W; col++) {
				if (bits & (1 << (FONT_GLYPH_W - 1 - col))) {
					fillBlock(buffer, pitchPixels, penX + col * scale, y + row * scale, scale, color);
				}
			}
		}

		penX += (FONT_GLYPH_W + GLYPH_SPACING) * scale;
	}
}

s32 textWidth5x7(const char *text, u32 scale)
{
	s32 len = 0;
	for (const char *p = text; *p; p++) len++;

	if (len == 0) return 0;

	// N glyphs of FONT_GLYPH_W each, with GLYPH_SPACING between them (not
	// after the last one).
	return (len * FONT_GLYPH_W + (len - 1) * GLYPH_SPACING) * scale;
}
