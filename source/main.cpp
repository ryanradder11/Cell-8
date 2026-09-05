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

	u32 frame = 0;
	while (running) {
		sysUtilCheckCallback();

		/* Cycle the clear color over time so it's visibly rendering,
		   not just a static screen. */
		u8 r = (u8) (frame & 0xff);
		u8 g = (u8) ((frame * 2) & 0xff);
		u8 b = (u8) ((frame * 3) & 0xff);
		u32 color = (r << 16) | (g << 8) | b;

		rsxSetClearColor(gGcmContext, color);
		rsxClearSurface(gGcmContext, GCM_CLEAR_R | GCM_CLEAR_G | GCM_CLEAR_B | GCM_CLEAR_A);

		flip();
		frame++;
	}

	printf("Cell-8: exiting...\n");
	return 0;
}
