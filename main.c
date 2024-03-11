#include "./rom.h"
#include "./cpu.h"

int main(void) {
    cpu c;
    cpu_init(&c);

    rom *r = rom_load("rom.nes");


    bool power = true;
    while (power) {
        if (c.reset == TIED_LOW) cpu_reset(&c);
        cpu_clock(&c);
    }

    rom_unload(r);
    return 0;
}
