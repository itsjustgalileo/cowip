// debug_tools.c
#include <stdio.h>
#include <stdlib.h>

#include "./cpu.h"

void throw_exception(const int error) {
    switch(error) {
    case ACCESS_VIOLATION:
        perror("access violation!\n");
        break;
    case SEGFAULT:
        perror("segmentation fault!\n");
        break;
    }
#ifdef _DEBUG
    EXCEPTION
#else
    exit(error);
#endif // _DEBUG
}

// Function to print the binary representation of a byte
void print_binary(unsigned char value) {
    for (int i = 7; i >= 0; i--) {
        putchar((value & (1 << i)) ? '1' : '0');
    }
}

// Debug print function for the cpu structure
void debug_print_CPU(const struct cpu *c) {
    printf("Instruction Register (IR): %02X\n", c->IR);
    printf("Accumulator (A): %02X\n", c->A);
    printf("Index Register X: %02X\n", c->X);
    printf("Index Register Y: %02X\n", c->Y);
    printf("Stack Pointer (SP): %02X\n", c->SP);
    
    printf("Processor Status Register (P): %02X (Binary: ", c->P);
    print_binary(c->P);
    printf(")\n");
    
    printf("  Carry (C): %d\n", c->C);
    printf("  Zero (Z): %d\n", c->Z);
    printf("  Interrupt (I): %d\n", c->I);
    printf("  Decimal (D): %d\n", c->D);
    printf("  Break (B): %d\n", c->B);
    printf("  Unused (U): %d\n", c->U);
    printf("  Overflow (V): %d\n", c->V);
    printf("  Negative (N): %d\n", c->N);
    
    printf("Program Counter (PC): %04X\n", c->PC);
    printf("Cycles: %d\n", c->cycles);
    printf("NMI: %02X\n", c->nmi);
    printf("Reset: %02X\n", c->reset);
    printf("IRQ: %02X\n", c->irq);
    printf("Address Bus: %04X\n", c->address_bus);
    printf("Address Relative: %04X\n", c->address_relative);
    printf("Data Bus: %02X\n", c->data_bus);
}