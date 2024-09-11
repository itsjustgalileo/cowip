#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "./board.h"

int main(void) {
    Board *b = board_init("./roms/nop.bin");
    if (b == NULL) {
        printf("failed to init board\n");
        return 5;
    }

    bool power = true;
    while (power) {
        __run(b);
        debug_print_CPU(b->c);
        system("clear");
    }

    board_shutdown(b);
    
    return 0;
}
