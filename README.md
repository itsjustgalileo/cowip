# COWIP

---

![text](cowip.webp)

---

## PRESENTATION

A 6502 cpu emulator.

illegal opcodes functions are still not implemented for the cpu.

this serves as a base for building 6502-based computers and consoles (C64, Apple II, NES...)

**THIS IS VERY BUGGY**

---

## PREREQUISITES

* C11 compiler

---

## SUPPORTED PLATFORMS

- [X] Windows
- [X] Linux
- [X] macOS

---

## HOW TO USE

```c
#include <stdio.h>

#include "./cpu.h"

int main(void) {
    cpu *c = cpu_init();
    if (c == NULL) {
        fprintf(stderr, "failed to initialize cpu\n");
        return 2;
    }
    cpu_reset(c);

    // load rom into memory here

    bool power = true;
    while (power) {
        do {
            cpu_clock(c);
        }while (!cpu_done(c));
    }

    cpu_shutdown(c);
    
    return 0;
}

```

---

## REFERENCES

* [6502 opcodes](https://www.masswerk.at/6502/6502_instruction_set.html)
