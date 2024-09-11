# COWIP

---

![text](cowip.webp)

---

## PRESENTATION

A 6502 cpu emulator. This has all addressing mode, legal & illegal opcodes implemented.

So far, this has only 2x32KB memory arrays to represent RAM and ROM. this serves as a boilerplate for building 6502-based computers and consoles (C64, Apple II, NES...) or even you own custom machine.

**THIS MIGHT BE BUGGY** So far, all issue I have encountered were from programs I was making not the CPU itself.

---

## PREREQUISITES

* C11 compiler
* Make (not a lot of source files so you can compile directly on the CLI, but make is just easier)

---

## SUPPORTED PLATFORMS

- [X] Windows
- [X] Linux
- [X] macOS

---

## HOW TO USE

```c
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "./board.h"

int main(void) {
    Board *b = board_init("./rom.bin");
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

```

```sh
make
./emulator <ROM FILE PATH>
# if no rom is provided a default ROM full of NOP is loaded
```

---

## REFERENCES

* [NesDev Wiki](https://www.nesdev.org/wiki/NES_reference_guide)
* [6502 inner workings](https://www.masswerk.at/6502/6502_instruction_set.html)
* [NES general info](https://www.copetti.org/writings/consoles/nes/)

---

## TODO

- [ ] RAM
- [ ] I/O
- [ ] VRAM
- [ ] AUDIO
- [ ] BANKS
- [ ] BIOS
- [ ] BASIC
