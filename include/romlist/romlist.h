#ifndef ROMLIST_H
#define ROMLIST_H

struct RomEntry {
	const char *name;
	const char *path;
};

extern const RomEntry ROM_LIST[];
extern const int ROM_COUNT;

#endif // ROMLIST_H
