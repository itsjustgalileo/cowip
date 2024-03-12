#include <stdio.h>

#include "./cpu.h"

int main(void) {
    cpu *c = cpu_init();
    if (c == NULL) {
        fprintf(stderr, "failed to initialize cpu\n");
        return 2;
    }
    cpu_reset(c);

    bool power = true;
    while (power) {
        do {
            cpu_clock(c);
        }while (!cpu_done(c));
    }

    cpu_shutdown(c);
    
    return 0;
}
