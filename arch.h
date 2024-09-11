#ifndef ARCH_H_
#define ARCH_H_

#include <stdbool.h>
#include <stdint.h>

#include "./debug_tools.h"

// Happy compiler
#define UNUSED(var) (void)var;

typedef uint8_t byte;
typedef uint16_t  word;
typedef word addr;

#define TIED_HIGH 0
#define TIED_LOW 1

// 1.79MHz
#define CLOCK_FREQUENCY 1798773

// Memory sections start address
#define ZERO_PAGE       0x0000
#define STACK_BASE      0x0100

#define ROM_BASE        0x8000

// Memory sections sizes
// TODO Separate RAM into different sections
#define RAM_SIZE         0x8000  // 32KB EEPROM AT28C256 INTERNAL RAM
#define ROM_SIZE         0x8000  // 32KB EEPROM AT28C256 PRG-ROM

#endif // !ARCH_H_
