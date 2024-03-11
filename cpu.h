#ifndef CPU_H_
#define CPU_H_

#ifndef __cplusplus
#define bool _Bool
#define true 1
#define false 0
#endif // !__cplusplus

typedef unsigned char byte;
typedef unsigned short word;
typedef word addr;

// interrupt vectors
#define NMI 0xFFFA // 0xFFFB
#define RESET 0xFFFC // 0xFFFD
#define IRQ 0xFFFE // 0xFFFF

#define TIED_HIGH 0
#define TIED_LOW 1

#define UNUSED(var) (void)var;

// 6502 CPU
typedef struct cpu {
    byte IR;      // instruction register
    byte A, X, Y; // GP registers
    byte P;       // stack pointer
    union {
        byte S; // status register
        struct {               
            byte C : 1; // carry
            byte Z : 1; // zero
            byte I : 1; // interrup
            byte D : 1; // decimal
            byte B : 1; // break
            byte U : 1; // unused
            byte V : 1; // overflow
            byte N : 1; // negative
        };
    };
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
byte cpu_read(addr address);
void cpu_write(addr address, byte data);

void cpu_set_flag(cpu *c, byte flag, bool condition);
byte cpu_get_flag(cpu *c, byte flag);

// cpu state
void cpu_init(cpu *c);
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

// ILLEGAL OPCODES USE AT YOUR OWN RISK
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