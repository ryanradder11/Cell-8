#ifndef ROMLIST_H
#define ROMLIST_H

// ROM list is populated purely by scanning for .ch8 files at runtime --
// no built-in/bundled list -- across these directories:
//   - HDD: /dev_hdd0/game/CELL80001/USRDIR/roms/ (created if missing --
//     CELL80001 must match this app's APPID in the Makefile)
//   - USB: /dev_usb000/roms/ through /dev_usb007/roms/ (all 8 possible
//     USB mass-storage slots -- a stick in any port, front or back,
//     lands in whichever slot is next free, so all are checked; slots
//     with nothing plugged in are silently skipped, not an error)

#define ROM_NAME_MAX 48
#define ROM_PATH_MAX 160
#define ROM_LIST_MAX 256

struct RomEntry {
	char name[ROM_NAME_MAX];
	char path[ROM_PATH_MAX];
};

extern RomEntry ROM_LIST[ROM_LIST_MAX];
extern int ROM_COUNT;

// Populates ROM_LIST/ROM_COUNT. Call once at startup, before the ROM
// selection menu is shown.
void romlist_init();

#endif // ROMLIST_H
