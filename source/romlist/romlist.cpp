#include "romlist/romlist.h"

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <dirent.h>
#include <sys/stat.h>



RomEntry ROM_LIST[ROM_LIST_MAX];
int ROM_COUNT = 0;

static void addRom(const char *name, const char *path)
{
	if (ROM_COUNT >= ROM_LIST_MAX) return; // silently drop past the cap

	strncpy(ROM_LIST[ROM_COUNT].name, name, ROM_NAME_MAX - 1);
	ROM_LIST[ROM_COUNT].name[ROM_NAME_MAX - 1] = '\0';

	strncpy(ROM_LIST[ROM_COUNT].path, path, ROM_PATH_MAX - 1);
	ROM_LIST[ROM_COUNT].path[ROM_PATH_MAX - 1] = '\0';

	ROM_COUNT++;
}

static bool hasCh8Extension(const char *filename)
{
	size_t len = strlen(filename);
	return len > 4 && strcasecmp(filename + len - 4, ".ch8") == 0;
}

// Sanitizes a filename into a font5x7-renderable display name: strips the
// .ch8 extension, uppercases, and replaces anything outside A-Z/0-9/-/space
// with a space, since font5x7 doesn't render brackets, parens, commas,
// periods, or lowercase.
static void sanitizeName(const char *filename, char *out, int outSize)
{
	int len = (int) strlen(filename);
	if (len > 4 && strcasecmp(filename + len - 4, ".ch8") == 0) {
		len -= 4;
	}

	int j = 0;
	for (int i = 0; i < len && j < outSize - 1; i++) {
		char c = filename[i];
		if (c >= 'a' && c <= 'z') c = (char) (c - 32); // uppercase
		bool ok = (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == ' ';
		out[j++] = ok ? c : ' ';
	}
	out[j] = '\0';
}

// Scans one directory for .ch8 files and appends them to ROM_LIST.
// dirPath must end with '/'. If the directory can't be opened at all
// (doesn't exist, no USB inserted, etc.) this just does nothing --
// callers decide whether that's worth creating the directory for first.
static void scanDirectory(const char *dirPath)
{
	DIR *dir = opendir(dirPath);
	if (!dir) return;

	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL) {
		if (!hasCh8Extension(entry->d_name)) continue;

		char name[ROM_NAME_MAX];
		sanitizeName(entry->d_name, name, sizeof(name));

		char path[ROM_PATH_MAX];
		snprintf(path, sizeof(path), "%s%s", dirPath, entry->d_name);

		addRom(name, path);
	}

	closedir(dir);
}

void romlist_init()
{
	ROM_COUNT = 0;

	// HDD: must match this app's actual APPID (Makefile's APPID :=
	// CELL80001) so this resolves to our own game's install dir once
	// packaged. Create it if missing so users have somewhere to drop
	// ROMs for next time; if the parent (.../USRDIR/) doesn't exist
	// either (e.g. running unpackaged straight from app_home during
	// development), mkdir just fails harmlessly and the following scan
	// finds nothing -- no error shown to the user either way.
	const char *hddRomsDir = "/dev_hdd0/game/CELL80001/USRDIR/roms/";
	struct stat st;
	if (stat(hddRomsDir, &st) != 0) {
		mkdir(hddRomsDir, 0777);
	}
	scanDirectory(hddRomsDir);

	// USB: a stick plugged into any port (front or back, any PS3 model)
	// gets mounted at the next free slot from /dev_usb000 to /dev_usb007
	// -- there's no fixed mapping from physical port to slot number, so
	// all 8 have to be checked. Only scan ones actually present; no USB
	// inserted in a given slot is a normal case, not an error --
	// stat() failing here is all we need to silently skip it.
	for (int i = 0; i < 8; i++) {
		char usbRomsDir[32];
		snprintf(usbRomsDir, sizeof(usbRomsDir), "/dev_usb00%d/roms/", i);
		if (stat(usbRomsDir, &st) == 0) {
			scanDirectory(usbRomsDir);
		}
	}
}
