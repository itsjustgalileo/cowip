#ifndef BOARD_H_
#define BOARD_H_

#include "./arch.h"
#include "./crystos.h"
#include "./cpu.h"

typedef struct Board {
    // Clock
    Clock clk;
    // CPU
    cpu *c;

    byte ram[RAM_SIZE];
    byte rom[ROM_SIZE];
} Board; 

Board *board_init(const char *rom_path);
void board_shutdown(Board *b);

byte board_read(Board *b, addr address);
void board_write(Board *b, addr address, byte data);

void __run(Board *b);

#endif // BOARD_H_
