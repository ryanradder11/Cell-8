#ifndef SOUND_H
#define SOUND_H

void soundInit();
void soundQuit();

// Called once per frame right after chip.sound_timer is decremented, to
// tell the audio thread whether a tone should be playing right now.
void soundSetActive(bool active);

#endif // SOUND_H
