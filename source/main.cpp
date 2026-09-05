#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ppu-types.h>

#include <sys/process.h>
#include <sysutil/sysutil.h>
#include <io/pad.h>

#include "rsxutil.h"
#include "font/font5x7.h"
#include "chip8/chip8.h"

SYS_PROCESS_PARAM(1001, 0x100000);

static u32 running = 0;

extern "C" {
static void program_exit_callback()
{
	finish();
}

static void sysutil_exit_callback(u64 status, u64 param, void *usrdata)
{
	(void) param;
	(void) usrdata;

	if (status == SYSUTIL_EXIT_GAME) {
		running = 0;
	}
}
}

// Platform glue for ~/projects/chip8-emulator (ported as-is into
// include/chip8, source/chip8 -- see chip8.h). The original targets
// SDL2 for display/input; these two functions are the PS3 replacements
// for its drawDisplay()/processInput(), same idea (blit the 64x32
// monochrome buffer as scaled squares; map keys to buttons), just against
// this project's RSX framebuffer and io/pad instead of an SDL window.

static void drawChip8Display(u32 *buffer, u32 pitchPixels, const Chip8 &chip, s32 originX, s32 originY, u32 scale)
{
	for (u32 y = 0; y < 32; y++) {
		u32 color = 0;
		for (u32 x = 0; x < 64; x++) {
			color = chip.gfx[y * 64 + x] ? 0x00ffffff : 0x00000000;
			for (u32 dy = 0; dy < scale; dy++) {
				for (u32 dx = 0; dx < scale; dx++) {
					buffer[(originY + y * scale + dy) * pitchPixels + (originX + x * scale + dx)] = color;
				}
			}
		}
	}
}

// Same key layout the original used on a keyboard (four rows of four),
// just mapped onto the DualShock's buttons instead -- there's no "as-is"
// precedent for this since the original never ran on a gamepad.
static void updateChip8Keys(Chip8 &chip, const padData &paddata)
{
	chip.keypad[0x1] = paddata.BTN_UP;
	chip.keypad[0x2] = paddata.BTN_DOWN;
	chip.keypad[0x3] = paddata.BTN_LEFT;
	chip.keypad[0xC] = paddata.BTN_RIGHT;

	chip.keypad[0x4] = paddata.BTN_TRIANGLE;
	chip.keypad[0x5] = paddata.BTN_CIRCLE;
	chip.keypad[0x6] = paddata.BTN_CROSS;
	chip.keypad[0xD] = paddata.BTN_SQUARE;

	chip.keypad[0x7] = paddata.BTN_L1;
	chip.keypad[0x8] = paddata.BTN_R1;
	chip.keypad[0x9] = paddata.BTN_L2;
	chip.keypad[0xE] = paddata.BTN_R2;

	chip.keypad[0xA] = paddata.BTN_SELECT;
	chip.keypad[0x0] = paddata.BTN_START;
	chip.keypad[0xB] = paddata.BTN_L3;
	chip.keypad[0xF] = paddata.BTN_R3;
}

int main(void)
{
	printf("Cell-8: RSX hello world starting...\n");

	initScreen();

	atexit(program_exit_callback);
	sysUtilRegisterCallback(SYSUTIL_EVENT_SLOT0, sysutil_exit_callback, NULL);

	running = 1;

	ioPadInit(7);

	// Intro screen: show the title + subtitle until the player presses X,
	// or the game is asked to exit (SYSUTIL_EXIT_GAME) while still on it.
	// Text is drawn with our own CPU-side 5x7 bitmap font (font5x7.cpp)
	// straight into the framebuffer -- no RSX shaders involved, since
	// cgcomp/Cg (needed for PSL1GHT's own shader-based debugfont_renderer)
	// isn't available on this ARM64 toolchain.
	const char *title = "CELL-8";
	const char *subtitle = "PRESS X TO START";
	const u32 titleScale = 6;
	const u32 subtitleScale = 3;

	padInfo padinfo;
	padData paddata;
	bool introRunning = true;

	while (introRunning && running) {
		sysUtilCheckCallback();

		ioPadGetInfo(&padinfo);
		for (int i = 0; i < MAX_PADS; i++) {
			if (padinfo.status[i]) {
				ioPadGetData(i, &paddata);
				if (paddata.BTN_CROSS) {
					introRunning = false;
				}
			}
		}

		// Per PSL1GHT convention, only ever write into the buffer that
		// isn't currently on screen -- flip()/curr_fb already track that.
		u32 *buf = color_buffer[curr_fb];
		u32 pitchPixels = color_pitch / 4;
		memset(buf, 0, display_height * color_pitch);

		s32 titleX = (display_width - textWidth5x7(title, titleScale)) / 2;
		s32 subtitleX = (display_width - textWidth5x7(subtitle, subtitleScale)) / 2;

		drawText5x7(buf, pitchPixels, titleX, display_height/2 - 40, title, 0x00ffffff, titleScale);
		drawText5x7(buf, pitchPixels, subtitleX, display_height/2 + 20, subtitle, 0x00aaaaaa, subtitleScale);

		flip();
	}

	// CHIP-8: ~/projects/chip8-emulator, ported as-is (see chip8.h/.cpp).
	// Same fixed ROM path the original loaded by default.
	Chip8 chip = {}; // zero-initialized; the original leaves this to
	                 // whatever garbage was on the stack, which is riskier
	                 // to carry over onto a different CPU architecture
	chip.pc = 0x200; // Start of most CHIP-8 programs
	// PS3 file I/O needs the /app_home/ VFS prefix to find files bundled
	// next to the executable -- a bare relative path (what the original
	// desktop version used) doesn't resolve here.
	loadROM("/app_home/roms/games/Figures.ch8", chip);

	const u32 chip8Scale = 10; // matches the original's drawDisplay() default
	s32 chip8OriginX = (display_width - 64 * chip8Scale) / 2;
	s32 chip8OriginY = (display_height - 32 * chip8Scale) / 2;

	while (running) {
		sysUtilCheckCallback();

		ioPadGetInfo(&padinfo);
		for (int i = 0; i < MAX_PADS; i++) {
			if (padinfo.status[i]) {
				ioPadGetData(i, &paddata);
				updateChip8Keys(chip, paddata);
			}
		}

		// The original ran emulateCycle() roughly 100x/sec (a free-running
		// loop with a 10ms SDL_Delay). Our loop is instead paced by
		// flip()'s vsync (~60Hz), so a couple of cycles per rendered
		// frame lands in the same ballpark.
		emulateCycle(chip);
		emulateCycle(chip);

		// 60Hz timers, decremented once per rendered frame.
		if (chip.delay_timer > 0) chip.delay_timer--;
		if (chip.sound_timer > 0) chip.sound_timer--; // no audio output (yet)

		// Redrawn every frame regardless of drawFlag (unlike the original):
		// with quad-buffering, only redrawing on drawFlag can leave stale
		// content (e.g. leftover intro text) in ring-buffer slots that
		// weren't touched during the most recent draw.
		u32 *buf = color_buffer[curr_fb];
		u32 pitchPixels = color_pitch / 4;
		memset(buf, 0, display_height * color_pitch);

		drawChip8Display(buf, pitchPixels, chip, chip8OriginX, chip8OriginY, chip8Scale);

		chip.drawFlag = false;

		flip();
	}

	printf("Cell-8: exiting...\n");
	return 0;
}
