// debug_tools.h
#ifndef DEBUG_TOOLS_H
#define DEBUG_TOOLS_H

#define EXCEPTION __builtin_trap()

#define ACCESS_VIOLATION 0xFF
#define SEGFAULT 0xFE


typedef struct cpu cpu;

void throw_exception(const int error);
void print_binary(unsigned char value);
void debug_print_CPU(const struct cpu *c);

#endif // DEBUG_TOOLS_H
