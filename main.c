#include "./rom.h"
#include "./cpu.h"

int main(void) {
    cpu c;
    cpu_init(&c);

    rom *r = rom_load("rom.bin");
    if(r == NULL) {
        fprintf(stderr, "failed to load rom file\n");
        return 1;
    }

    for (size_t i = 0; i < r->size; i++) {
        cpu_write(i, r->bytes[i]);
    }

    bool power = true;
    while (power) {
        do {
            cpu_clock(&c);
        }while (!cpu_done(&c));
    }

    rom_unload(r);
    return 0;
}
