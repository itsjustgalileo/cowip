#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "./board.h"


Board *board_init(const char *rom_path) {
    Board *b = (Board *)malloc(sizeof(Board));
    if (b == NULL) {
        perror("failed to allocate memory for board\n");
        return NULL;
    }

    // Initialize clock frequency
    b->clk.frequency = CLOCK_FREQUENCY;

    // Initialize CPU
    b->c = cpu_init();
    if (b->c == NULL) {
        free(b);
        return NULL;
    }
    b->c->bc = b;

    FILE *file;
    size_t bytesRead;

    // Open the binary file in read-binary mode
    file = fopen(rom_path, "rb");
    if (file == NULL) {
        perror("Error opening file");
        return NULL;
    }

    // Read the file content into the memory array
    bytesRead = fread(b->rom, 1, ROM_SIZE, file);
    if (bytesRead != ROM_SIZE) {
        if (feof(file)) {
            printf("Warning: The file is smaller than the expected size.\n");
        } else if (ferror(file)) {
            perror("Error reading file");
        }
        fclose(file);
        return NULL;
    }

    // Close the file
    fclose(file);

    // Reset the CPU
    cpu_reset(b->c);
    return b;
}

void board_shutdown(Board *b) {
    if (b == NULL) {
        return;
    }
    cpu_shutdown(b->c);
    free(b);
}

byte board_read(Board *b, addr address) {
    if(address >= 0 && address < RAM_SIZE)
        return b->ram[address];
    return b->rom[address - ROM_BASE];
}

void board_write(Board *b, addr address, byte data) {
    if(address >= 0 && address < RAM_SIZE)
        b->ram[address] = data;
    throw_exception(ACCESS_VIOLATION);
}

void __run(Board *b) {
    do {
        tick(&b->clk, cpu_clock, b);
    } while (!cpu_done(b->c));
}
