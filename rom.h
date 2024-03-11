#ifndef ROM_H_
#define ROM_H_

#include <stdlib.h>

typedef struct {
    unsigned char *bytes;
    size_t size;
} rom;

rom *rom_load(const char *path);
void rom_unload(rom *r);
void rom_dump(rom *r);

#endif // !ROM_H_