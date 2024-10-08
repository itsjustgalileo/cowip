#ifndef CPU_H_
#define CPU_H_

#include "./arch.h"

#define CPU_JAM 1 // THIS IS ALWAYS 1

// interrupt vectors
#define NMI 0xFFFA // 0xFFFB
#define RESET 0xFFFC // 0xFFFD
#define IRQ 0xFFFE // 0xFFFF

// processor status flags
#define FLAG_C 0x01  // Carry flag
#define FLAG_Z 0x02  // Zero flag
#define FLAG_I 0x04  // Interrupt disable
#define FLAG_D 0x08  // Decimal mode
#define FLAG_B 0x10  // Break command
#define FLAG_U 0x20  // Unused
#define FLAG_V 0x40  // Overflow flag
#define FLAG_N 0x80  // Negative flag

typedef struct Board Board;

// 8-BIT CPU
typedef struct cpu {
    // connection to motherboard
    Board *bc;
    
    byte IR;       // instruction register
    byte A;        // accumulator
    byte X, Y;     // index registers
    byte SP;       // stack pointer
                   // I know that all 1 byte register are written with one letter
                   // but technically this refers to $0100 + the stack pointer!
                   // so it's 2 bytes
    byte P;
    word PC; // program counter

    byte cycles; // internal cycles

    byte nmi;
    byte reset;
    byte irq;

    addr address_bus;
    addr address_relative;
    byte data_bus;

    struct code_t {
        char *str;
        byte (*addressing_mode)(struct cpu *c);
        byte (*opcode)(struct cpu *c);
        byte cycles;
    } code[0x0100];
} cpu;

// memory operations
byte cpu_read(cpu *c, addr address);
void cpu_write(cpu *c, addr address, byte data);

// cpu state
void cpu_set_flag(cpu *c, byte flag, bool condition);
byte cpu_get_flag(cpu *c, byte flag);

cpu *cpu_init(void);
void cpu_shutdown(cpu *c);

void cpu_clock(cpu *c);
bool cpu_done(cpu *c);

void cpu_nmi(cpu *c);
void cpu_reset(cpu *c);
void cpu_irq(cpu *c);

void cpu_code(cpu *c);
byte cpu_decode(cpu *c);

// ADDRESSING MODES
byte IMP(cpu *c);
byte ACC(cpu *c);
byte IMM(cpu *c);
byte ZPG(cpu *c);
byte ZPX(cpu *c);
byte ZPY(cpu *c);
byte ABS(cpu *c);
byte ABX(cpu *c);
byte ABY(cpu *c);
byte IND(cpu *c);
byte IZY(cpu *c);
byte IZX(cpu *c);
byte REL(cpu *c);

// LEGAL OPCODES
byte ADC(cpu *c);
byte AND(cpu *c);
byte ASL(cpu *c);
byte BCC(cpu *c);
byte BCS(cpu *c);
byte BEQ(cpu *c);
byte BIT(cpu *c);
byte BMI(cpu *c);
byte BNE(cpu *c);
byte BPL(cpu *c);
byte BRK(cpu *c);
byte BVC(cpu *c);
byte BVS(cpu *c);
byte CLC(cpu *c);
byte CLD(cpu *c);
byte CLI(cpu *c);
byte CLV(cpu *c);
byte CMP(cpu *c);
byte CPX(cpu *c);
byte CPY(cpu *c);
byte DEC(cpu *c);
byte DEX(cpu *c);
byte DEY(cpu *c);
byte EOR(cpu *c);
byte INC(cpu *c);
byte INX(cpu *c);
byte INY(cpu *c);
byte JMP(cpu *c);
byte JSR(cpu *c);
byte LDA(cpu *c);
byte LDX(cpu *c);
byte LDY(cpu *c);
byte LSR(cpu *c);
byte NOP(cpu *c);
byte ORA(cpu *c);
byte PHA(cpu *c);
byte PHP(cpu *c);
byte PLA(cpu *c);
byte PLP(cpu *c);
byte ROL(cpu *c);
byte ROR(cpu *c);
byte RTI(cpu *c);
byte RTS(cpu *c);
byte SBC(cpu *c);
byte SEC(cpu *c);
byte SED(cpu *c);
byte SEI(cpu *c);
byte STA(cpu *c);
byte STX(cpu *c);
byte STY(cpu *c);
byte TAX(cpu *c);
byte TAY(cpu *c);
byte TSX(cpu *c);
byte TXA(cpu *c);
byte TXS(cpu *c);
byte TYA(cpu *c);

// ILLEGAL OPCODES
byte ALR(cpu *c);
byte ANC(cpu *c);
byte ANE(cpu *c);
byte ARR(cpu *c);
byte DCP(cpu *c);
byte ISC(cpu *c);
byte LAS(cpu *c);
byte LAX(cpu *c);
byte LXA(cpu *c);
byte RLA(cpu *c);
byte RRA(cpu *c);
byte SAX(cpu *c);
byte SBX(cpu *c);
byte SHA(cpu *c);
byte SHX(cpu *c);
byte SHY(cpu *c);
byte SLO(cpu *c);
byte SRE(cpu *c);
byte TAS(cpu *c);
byte USBC(cpu *c);
byte NOP_undoc(cpu *c);
byte JAM (cpu *c);

#endif // !CPU_H_
