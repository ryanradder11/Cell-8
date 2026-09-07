#include <audio/audio.h>
#include <sys/thread.h>

#include "sound/sound.h"

static volatile bool soundActive = false;

static volatile bool audioThreadRunning = false;
static sys_ppu_thread_t audioThreadId;
static u32 audioPortNum;
static f32 *audioBase;
static sys_event_queue_t audioEventQueue;
static sys_ipc_key_t audioEventQueueKey;

static void audioThreadEntry(void *arg)
{
	(void) arg;

	double phase = 0.0;
	const double phaseStep = 432.0 / 48000.0; // 432Hz at the PS3's fixed 48kHz audio rate

	u32 nextBlock = 0;

	while (audioThreadRunning) {
		sys_event_t event;

		//Blocks thread until event is received
		sysEventQueueReceive(audioEventQueue, &event, 0);

		f32 *block = audioBase + nextBlock * AUDIO_PORT_2CH * AUDIO_BLOCK_SAMPLES;

		bool active = soundActive;

		for (u32 i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
			f32 sample = 0.0f;
			//1 hardest up, -1 hardest down. +0.2 in first half of phase, -0.2 in second half of phase
			if (active) sample = (phase < 0.5) ? 0.2f : -0.2f;

			block[i * 2 + 0] = sample;
			block[i * 2 + 1] = sample;

			phase += phaseStep;
			if (phase >= 1.0) phase -= 1.0;
		}

		nextBlock = (nextBlock + 1) % AUDIO_BLOCK_8;
	}

	sysThreadExit(0);
}

void soundInit()
{
	audioInit();

	audioPortParam params;
	params.numChannels = AUDIO_PORT_2CH;
	params.numBlocks = AUDIO_BLOCK_8;
	params.attrib = 0;

	audioPortOpen(&params, &audioPortNum);

	audioPortConfig config;
	audioGetPortConfig(audioPortNum, &config);
	audioBase = (f32*)(uintptr_t) config.audioDataStart;

	//creates and sets eventQue, PS3 sends notification to this thread every 256 samples on ready
	audioCreateNotifyEventQueue(&audioEventQueue, &audioEventQueueKey);
	audioSetNotifyEventQueue(audioEventQueueKey);
	sysEventQueueDrain(audioEventQueue);

	audioPortStart(audioPortNum);

	audioThreadRunning = true;

	//create PPU thread, set prio to mid(1500) and give 8kb of stack size
	sysThreadCreate(&audioThreadId, audioThreadEntry, NULL, 1500, 8 * 1024, THREAD_JOINABLE, (char*)"sound_thread");
}

void soundQuit()
{
	audioThreadRunning = false;
	u64 exitCode;
	sysThreadJoin(audioThreadId, &exitCode);

	audioPortStop(audioPortNum);
	audioRemoveNotifyEventQueue(audioEventQueueKey);
	audioPortClose(audioPortNum);
	sysEventQueueDestroy(audioEventQueue, 0);
	audioQuit();
}

void soundSetActive(bool active)
{
	soundActive = active;
}
