#ifndef SOUND_H
#define SOUND_H

void soundInit();
void soundQuit();

//call once per frame to simply tell if a tone should be playing yes/no
void soundSetActive(bool active);

#endif
