#include "./cpu.h"

// TODO: separate into different memory sections
static byte memory[64 * 1024];

byte cpu_read(addr address) {
    return memory[address];
}

void cpu_write(addr address, byte data) {
    memory[address] = data;
}

void cpu_set_flag(cpu *c, byte flag, bool condition) {
    if (condition) {
        c->S |= flag;
    } else {
        c->S &= ~flag;
    }
}

byte cpu_get_flag(cpu *c, byte flag) {
    return (c->S & flag) ? 1 : 0;
}

void cpu_init(cpu* c) {
    cpu_code(c);

    // reset cycle 0
    c->cycles = 0;
    c->address_bus = 0x00FF;
    c->data_bus = 0x00;
    c->PC = 0x00FF;
    c->IR = 0x00;
    c->A = 0xAA;
    c->X = 0x00;
    c->Y = 0x00;
    c->P = 0x00;
    c->S = 0b00000010;
    c->nmi = TIED_HIGH;
    c->reset = TIED_LOW;
    c->irq = TIED_HIGH;

    cpu_reset(c);
}

bool cpu_done(cpu *c) {
    return c->cycles == 0;
}

byte cpu_decode(cpu *c) {
    if(c->code[c->IR].addressing_mode != &IMM)
        c->data_bus = cpu_read(c->address_bus);
    return c->data_bus;
}

void cpu_nmi(cpu *c) {
    cpu_write(0x0100 + c->P, (c->PC >> 8) & 0x00FF);
	c->P--;
	cpu_write(0x0100 + c->P, c->PC & 0x00FF);
	c->P--;

	cpu_set_flag(c, c->B, 0);
	cpu_set_flag(c, c->U, 1);
	cpu_set_flag(c, c->I, 1);
	cpu_write(0x0100 + c->P, c->S);
	c->P--;

	c->address_bus = 0xFFFA;
	addr lo = cpu_read(c->address_bus + 0);
	addr hi = cpu_read(c->address_bus + 1);
	c->PC = (hi << 8) | lo;

	c->cycles = 8;
}

void cpu_reset(cpu* c) {
    // Assume reset is already initiated; prepare for the reset sequence
    
    c->S = 0xFD; // Stack pointer usually set to 0xFF on reset
    c->P = 0x24; // Status register with UNUSED and IRQ disable flags set
        
    // Read reset vector into PC
    word lowByte = cpu_read(RESET);
    word highByte = cpu_read(RESET + 1);
    c->PC = (highByte << 8) | lowByte;

    c->data_bus = cpu_read(c->PC);
    // Reset complete; further initialization as needed
    c->reset = TIED_HIGH; // Indicate reset completed
    c->cycles = 0; // Ready for next operation
    
}

void cpu_irq(cpu *c) {
    if (cpu_get_flag(c, c->I) == 0)
	{
		// Push the program counter to the stack. It's 16-bits dont
		// forget so that takes two pushes
		cpu_write(0x0100 + c->P, (c->PC >> 8) & 0x00FF);
		c->P--;
		cpu_write(0x0100 + c->P, c->PC & 0x00FF);
		c->P--;

		// Then Push the status register to the stack
		cpu_set_flag(c, c->B, 0);
		cpu_set_flag(c, c->U, 1);
		cpu_set_flag(c, c->I, 1);
		cpu_write(0x0100 + c->P, c->S);
		c->P--;

		// Read new program counter location from fixed address
		c->address_bus = 0xFFFE;
		addr lo = cpu_read(c->address_bus + 0);
		addr hi = cpu_read(c->address_bus + 1);
		c->PC = (hi << 8) | lo;

		// IRQs take time
		c->cycles = 7;
	}
}

void cpu_clock(cpu *c) {
    if (c->cycles == 0) {
        c->IR = cpu_read(c->PC);
        cpu_set_flag(c, c->U, true);
        c->PC++;
        c->cycles = c->code[c->IR].cycles;
        byte cycle1 = (c->code[c->IR].addressing_mode)(c);
        byte cycle2 = (c->code[c->IR].opcode)(c);
        c->cycles += (cycle1 & cycle2);
        cpu_set_flag(c, c->U, true);
    }
    c->cycles--;
}

// ADDRESSING MODES
byte IMP(cpu *c) {
    // basically does not add extra cycles
    // stability depends on opcodes
    UNUSED(c);
    return 0;
}

byte ACC(cpu *c) {
    // For accumulator addressing, the operation is performed directly on the accumulator register.
    // Therefore, we don't need to read or write to memory, and the data_bus can be directly set to or from the accumulator.
    c->data_bus = c->A; // Set data_bus to the value of the accumulator for reading operations
    return 0; // Return 0 as this operation doesn't affect page boundary crossing or add extra cycles
}

byte IMM(cpu *c) {
    c->address_bus = c->PC++;
    return 0;
}

byte ZPG(cpu *c) {
    c->address_bus = cpu_read(c->PC);
    c->PC++;
	c->address_bus &= 0x00FF;	return 0;
}

byte ZPX(cpu *c) { 
    c->address_bus = cpu_read(c->PC + c->X);
    c->PC++;
	c->address_bus &= 0x00FF;
    return 0; 
}

byte ZPY(cpu *c) { 
    c->address_bus = cpu_read(c->PC + c->Y);
    c->PC++;
	c->address_bus &= 0x00FF;
    return 0; 
}

byte ABS(cpu *c) {
    // Read the low byte first, then increment the PC
    byte lo = cpu_read(c->PC);
    c->PC++;
    // Read the high byte next, PC now points to the next instruction or data byte
    byte hi = cpu_read(c->PC);
    c->PC++;

    // Combine the low and high bytes into the address_bus, ensuring little-endian order
    c->address_bus = (hi << 8) | lo;

    return 0;
}

byte ABX(cpu *c) {
    addr lo = cpu_read(c->PC);
	c->PC++;
	addr hi = cpu_read(c->PC);
	c->PC++;

	c->address_bus = (hi << 8) | lo;
	c->address_bus += c->X;

	if ((c->address_bus & 0xFF00) != (hi << 8))
		return 1;
	else
		return 0;
}

byte ABY(cpu *c) { 
    addr lo = cpu_read(c->PC);
	c->PC++;
	addr hi = cpu_read(c->PC);
	c->PC++;

	c->address_bus = (hi << 8) | lo;
	c->address_bus += c->Y;

	if ((c->address_bus & 0xFF00) != (hi << 8))
		return 1;
	else
		return 0;
}

byte IND(cpu *c) {
    addr ptr_lo = cpu_read(c->PC);
    c->PC++;

    addr ptr_hi = cpu_read(c->PC);
    c->PC++;
    
    addr ptr = (ptr_hi << 8) | ptr_lo;

    // Handle page boundary bug
    if ((ptr & 0x00FF) == 0x00FF) {
        c->address_bus = (cpu_read(ptr & 0xFF00) << 8) | cpu_read(ptr);
    } else {
        c->address_bus = (cpu_read(ptr + 1) << 8) | cpu_read(ptr);
    }

    return 0;
}

byte IZY(cpu *c) { 
    addr tmp = cpu_read(c->PC++);

    addr lo = cpu_read(tmp & 0xFF);
    addr hi = cpu_read((tmp + 1) & 0xFF);

    c->address_bus = (hi << 8) | lo;
    c->address_bus += c->Y;

    if ((c->address_bus & 0xFF00) != (hi << 8)) {
        return 1;
    } else {
        return 0;
    }
    return 0; 
}

byte IZX(cpu *c) { 
    addr tmp = cpu_read(c->PC);
    c->PC++;

    addr lo = cpu_read((addr)(tmp + (addr)c->X) & 0xFF);
    addr hi = cpu_read((addr)(tmp + (addr)c->X + 1) & 0xFF);

    c->address_bus = (hi << 8) | lo;

    return 0; 
}

byte REL(cpu *c) { 
    c->address_relative = cpu_read(c->PC);
    c->PC++;
    if (c->address_relative & 0x80) 
        c->address_relative |= 0xFF00;
    return 0; 
}

// LEGAL OPCODES
byte ADC(cpu *c) {
    // Grab the data that we are adding to the accumulator
	cpu_decode(c);
	
	// Add is performed in 16-bit domain for emulation to capture any
	// carry bit, which will exist in bit 8 of the 16-bit word
	byte temp = (addr)c->A + (addr)c->data_bus + (addr)cpu_get_flag(c, c->C);
	
	// The carry flag out exists in the high byte bit 0
	cpu_set_flag(c, c->C, temp > 255);
	
	// The Zero flag is set if the result is 0
	cpu_set_flag(c, c->Z, (temp & 0x00FF) == 0);
	
	// The signed Overflow flag is set based on all that up there! :D
	cpu_set_flag(c, c->V, (~((addr)c->A ^ (addr)c->data_bus) & ((addr)c->A ^ (addr)temp)) & 0x0080);
	
	// The negative flag is set to the most significant bit of the result
	cpu_set_flag(c, c->N, temp & 0x80);
	
	// Load the result into the accumulator (it's 8-bit dont forget!)
	c->A = temp & 0x00FF;
	
	// This instruction has the potential to require an additional clock cycle
	return 1;
}

byte AND(cpu *c) {
    byte data = cpu_decode(c);
    c->A &= data;

    if(c->A == 0x00) {
        c->Z = 1;
    }

    if (c->A & 0x80) {
        c->N = 1;
    }
    
    return 1;
}

byte ASL(cpu *c) {
    cpu_decode(c);
	byte temp = (addr)c->data_bus << 1;
	cpu_set_flag(c, c->C, (temp & 0xFF00) > 0);
	cpu_set_flag(c, c->Z, (temp & 0x00FF) == 0x00);
	cpu_set_flag(c, c->N, temp & 0x80);
	if (c->code[c->IR].addressing_mode == &IMP)
		c->A = temp & 0x00FF;
	else
		cpu_write(c->address_bus, temp & 0x00FF);
    return 0;
}

byte BCC(cpu *c) {
    if (cpu_get_flag(c, c->C) == 0)
	{
		c->cycles++;
		c->address_bus = c->PC + c->address_relative;
		
		if((c->address_bus & 0xFF00) != (c->PC & 0xFF00))
			c->cycles++;
		
		c->PC = c->address_bus;
	}
    return 0;
}

byte BCS(cpu *c) {
    if (cpu_get_flag(c, c->C) == 1)
	{
		c->cycles++;
		c->address_bus = c->PC + c->address_relative;

		if ((c->address_bus & 0xFF00) != (c->PC & 0xFF00))
			c->cycles++;

		c->PC = c->address_bus;
	}
	return 0;
}

byte BEQ(cpu *c) {
    if (cpu_get_flag(c, c->Z) == 1)
	{
		c->cycles++;
		c->address_bus = c->PC + c->address_relative;

		if ((c->address_bus & 0xFF00) != (c->PC & 0xFF00))
			c->cycles++;

		c->PC = c->address_bus;
	}
	return 0;
}

byte BIT(cpu *c) {
    cpu_decode(c);
	byte temp = c->A & c->data_bus;
	cpu_set_flag(c, c->Z, (temp & 0x00FF) == 0x00);
	cpu_set_flag(c, c->N, c->data_bus & (1 << 7));
	cpu_set_flag(c, c->V, c->data_bus & (1 << 6));
	return 0;
}

byte BMI(cpu *c) {
    if (cpu_get_flag(c, c->N) == 1)
	{
		c->cycles++;
		c->address_bus = c->PC + c->address_relative;

		if ((c->address_bus & 0xFF00) != (c->PC & 0xFF00))
			c->cycles++;

		c->PC = c->address_bus;
	}
	return 0;
}

byte BNE(cpu *c) {
    if (cpu_get_flag(c, c->Z) == 0)
	{
		c->cycles++;
		c->address_bus = c->PC + c->address_relative;

		if ((c->address_bus & 0xFF00) != (c->PC & 0xFF00))
			c->cycles++;

		c->PC = c->address_bus;
	}
	return 0;
}

byte BPL(cpu *c) {
    if (cpu_get_flag(c, c->N) == 0)
	{
		c->cycles++;
		c->address_bus = c->PC + c->address_relative;

		if ((c->address_bus & 0xFF00) != (c->PC & 0xFF00))
			c->cycles++;

		c->PC = c->address_bus;
	}
	return 0;
}

byte BRK(cpu *c) {
    c->PC++;
	
	cpu_set_flag(c, c->I, 1);
	cpu_write(0x0100 + c->P, (c->PC >> 8) & 0x00FF);
	c->P--;
	cpu_write(0x0100 + c->P, c->PC & 0x00FF);
	c->P--;

	cpu_set_flag(c, c->B, 1);
	cpu_write(0x0100 + c->P, c->S);
	c->P--;
	cpu_set_flag(c, c->B, 0);

	c->PC = (addr)cpu_read(0xFFFE) | ((addr)cpu_read(0xFFFF) << 8);
	return 0;
}

byte BVC(cpu *c) {
    if (cpu_get_flag(c, c->V) == 0)
	{
		c->cycles++;
		c->address_bus = c->PC + c->address_relative;

		if ((c->address_bus & 0xFF00) != (c->PC & 0xFF00))
			c->cycles++;

		c->PC = c->address_bus;
	}
	return 0;
}

byte BVS(cpu *c) {
    if (cpu_get_flag(c, c->V) == 1)
	{
		c->cycles++;
		c->address_bus = c->PC + c->address_relative;

		if ((c->address_bus & 0xFF00) != (c->PC & 0xFF00))
			c->cycles++;

		c->PC = c->address_bus;
	}
	return 0;
}

byte CLC(cpu *c) {
    cpu_set_flag(c, c->C, false);
    return 0;
}

byte CLD(cpu *c) {
    cpu_set_flag(c, c->D, false);
    return 0;
}

byte CLI(cpu *c) {
    cpu_set_flag(c, c->I, false);
    return 0;
}

byte CLV(cpu *c) {
    cpu_set_flag(c, c->V, false);
    return 0;
}

byte CMP(cpu *c) {
	cpu_decode(c);
	byte temp = (addr)c->A - (addr)c->data_bus;
	cpu_set_flag(c, c->C, c->A >= c->data_bus);
	cpu_set_flag(c, c->Z, (temp & 0x00FF) == 0x0000);
	cpu_set_flag(c, c->N, temp & 0x0080);
	return 1;
}

byte CPX(cpu *c) {
    cpu_decode(c);
	byte temp = (addr)c->X - (addr)c->data_bus;
	cpu_set_flag(c, c->C, c->X >= c->data_bus);
	cpu_set_flag(c, c->Z, (temp & 0x00FF) == 0x0000);
	cpu_set_flag(c, c->N, temp & 0x0080);
	return 0;
}

byte CPY(cpu *c) {
    cpu_decode(c);
	byte temp = (addr)c->Y - (addr)c->data_bus;
	cpu_set_flag(c, c->C, c->Y >= c->data_bus);
	cpu_set_flag(c, c->Z, (temp & 0x00FF) == 0x0000);
	cpu_set_flag(c, c->N, temp & 0x0080);
	return 0;
}

byte DEC(cpu *c) {
    cpu_decode(c);
	byte temp = c->data_bus - 1;
	cpu_write(c->address_bus, temp & 0x00FF);
	cpu_set_flag(c, c->Z, (temp & 0x00FF) == 0x0000);
	cpu_set_flag(c, c->N, temp & 0x0080);
	return 0;
}

byte DEX(cpu *c) {
    c->X--;
	cpu_set_flag(c, c->Z, c->X == 0x00);
	cpu_set_flag(c, c->N, c->X & 0x80);
	return 0;
}

byte DEY(cpu *c) {
    c->Y--;
	cpu_set_flag(c, c->Z, c->Y == 0x00);
	cpu_set_flag(c, c->N, c->Y & 0x80);
	return 0;
}

byte EOR(cpu *c) {
    cpu_decode(c);
	c->A = c->A ^ c->data_bus;	
	cpu_set_flag(c, c->Z, c->A == 0x00);
	cpu_set_flag(c, c->N, c->A & 0x80);
	return 1;
}

byte INC(cpu *c) {
    cpu_decode(c);
	byte temp = c->data_bus + 1;
	cpu_write(c->address_bus, temp & 0x00FF);
	cpu_set_flag(c, c->Z, (temp & 0x00FF) == 0x0000);
	cpu_set_flag(c, c->N, temp & 0x0080);
	return 0;
}

byte INX(cpu *c) {
    c->X++;
	cpu_set_flag(c, c->Z, c->X == 0x00);
	cpu_set_flag(c, c->N, c->X & 0x80);
	return 0;
}

byte INY(cpu *c) {
    c->Y++;
	cpu_set_flag(c, c->Z, c->X == 0x00);
	cpu_set_flag(c, c->N, c->X & 0x80);
	return 0;
}

byte JMP(cpu *c) {
    c->PC = c->address_bus;
    return 0;
}

byte JSR(cpu *c) {
    c->PC--;

	cpu_write(0x0100 + c->P, (c->PC >> 8) & 0x00FF);
	c->P--;
	cpu_write(0x0100 + c->P, c->PC & 0x00FF);
	c->P--;

	c->PC = c->address_bus;
	return 0;
}

byte LDA(cpu *c) {
    cpu_decode(c);
	c->A = c->data_bus;
	cpu_set_flag(c, c->Z, c->A == 0x00);
	cpu_set_flag(c, c->N, c->A & 0x80);
	return 1;
}

byte LDX(cpu *c) {
    cpu_decode(c);
	c->X = c->data_bus;
	cpu_set_flag(c, c->Z, c->X == 0x00);
	cpu_set_flag(c, c->N, c->X & 0x80);
	return 1;
}

byte LDY(cpu *c) {
    cpu_decode(c);
	c->Y = c->data_bus;
	cpu_set_flag(c, c->Z, c->Y == 0x00);
	cpu_set_flag(c, c->N, c->Y & 0x80);
	return 1;
}

byte LSR(cpu *c) {
    cpu_decode(c);
	cpu_set_flag(c, c->C, c->data_bus & 0x0001);
	byte temp = c->data_bus >> 1;	
	cpu_set_flag(c, c->Z, (temp & 0x00FF) == 0x0000);
	cpu_set_flag(c, c->N, temp & 0x0080);
	if (c->code[c->IR].addressing_mode == &IMP)
		c->A = temp & 0x00FF;
	else
		cpu_write(c->address_bus, temp & 0x00FF);
	return 0;
}

byte NOP(cpu *c) {
    switch (c->IR) {
	case 0x1C:
	case 0x3C:
	case 0x5C:
	case 0x7C:
	case 0xDC:
	case 0xFC:
		return 1;
		break;
	}
	return 0;
}

byte ORA(cpu *c) {
    cpu_decode(c);
	c->A = c->A | c->data_bus;
	cpu_set_flag(c, c->Z, c->A == 0x00);
	cpu_set_flag(c, c->N, c->A & 0x80);
	return 1;
}

byte PHA(cpu *c) {
    cpu_write(0x0100 + c->P, c->A);
	c->P--;
	return 0;
}

byte PHP(cpu *c) {
    cpu_write(0x0100 + c->P, c->S | c->B | c->U);
	cpu_set_flag(c, c->B, 0);
	cpu_set_flag(c, c->U, 0);
	c->P--;
	return 0;
}

byte PLA(cpu *c) {
    c->P++;
	c->A = cpu_read(0x0100 + c->P);
	cpu_set_flag(c, c->Z, c->A == 0x00);
	cpu_set_flag(c, c->N, c->A & 0x80);
	return 0;
}

byte PLP(cpu *c) {
    c->P++;
	c->S = cpu_read(0x0100 + c->P);
	cpu_set_flag(c, c->U, 1);
	return 0;
}

byte ROL(cpu *c) {
    cpu_decode(c);
	byte temp = (addr)(c->data_bus << 1) | cpu_get_flag(c, c->C);
	cpu_set_flag(c, c->C, temp & 0xFF00);
	cpu_set_flag(c, c->Z, (temp & 0x00FF) == 0x0000);
	cpu_set_flag(c, c->N, temp & 0x0080);
	if (c->code[c->IR].addressing_mode == &IMP)
		c->A = temp & 0x00FF;
	else
		cpu_write(c->address_bus, temp & 0x00FF);
	return 0;
}

byte ROR(cpu *c) {
    cpu_decode(c);
	byte temp = (addr)(cpu_get_flag(c, c->C) << 7) | (c->data_bus >> 1);
	cpu_set_flag(c, c->C, c->data_bus & 0x01);
	cpu_set_flag(c, c->Z, (temp & 0x00FF) == 0x00);
	cpu_set_flag(c, c->N, temp & 0x0080);
	if (c->code[c->IR].addressing_mode == &IMP)
		c->A = temp & 0x00FF;
	else
		cpu_write(c->address_bus, temp & 0x00FF);
	return 0;
}

byte RTI(cpu *c) {
    c->P++;
	c->S = cpu_read(0x0100 + c->P);
	c->S &= ~c->B;
	c->S &= ~c->U;

	c->P++;
	c->PC = (addr)cpu_read(0x0100 + c->P);
	c->P++;
	c->PC |= (addr)cpu_read(0x0100 + c->P) << 8;
	return 0;
}

byte RTS(cpu *c) {
    c->P++;
	c->PC = (addr)cpu_read(0x0100 + c->P);
	c->P++;
	c->PC |= (addr)cpu_read(0x0100 + c->P) << 8;
	
	c->PC++;
	return 0;
}

byte SBC(cpu *c) {
    cpu_decode(c);
	
	// Operating in 16-bit domain to capture carry out
	
	// We can invert the bottom 8 bits with bitwise xor
	addr value = ((addr)c->data_bus) ^ 0x00FF;
	
	// Notice this is exactly the same as addition from here!
	byte temp = (addr)c->A + value + (addr)cpu_get_flag(c, c->C);
	cpu_set_flag(c, c->C, temp & 0xFF00);
	cpu_set_flag(c, c->Z, ((temp & 0x00FF) == 0));
	cpu_set_flag(c, c->V, (temp ^ (addr)c->A) & (temp ^ value) & 0x0080);
	cpu_set_flag(c, c->N, temp & 0x0080);
	c->A = temp & 0x00FF;
	return 1;
}

byte SEC(cpu *c) {
    cpu_set_flag(c, c->C, true);
    return 0;
}

byte SED(cpu *c) {
    cpu_set_flag(c, c->D, true);
    return 0;
}

byte SEI(cpu *c) {
    cpu_set_flag(c, c->I, true);
    return 0;
}

byte STA(cpu *c) {
    cpu_write(c->address_bus, c->A);
    return 0;
}

byte STX(cpu *c) {
    cpu_write(c->address_bus, c->X);
    return 0;
}

byte STY(cpu *c) {
    cpu_write(c->address_bus, c->Y);
    return 0;
}

byte TAX(cpu *c) {
    c->X = c->A;
	cpu_set_flag(c, c->Z, c->X == 0x00);
	cpu_set_flag(c, c->N, c->X & 0x80);
	return 0;
}

byte TAY(cpu *c) {
    c->Y = c->A;
	cpu_set_flag(c, c->Z, c->Y == 0x00);
	cpu_set_flag(c, c->N, c->Y & 0x80);
	return 0;
}

byte TSX(cpu *c) {
    c->X = c->P;
	cpu_set_flag(c, c->Z, c->X == 0x00);
	cpu_set_flag(c, c->N, c->X & 0x80);
	return 0;
}

byte TXA(cpu *c) {
    c->A = c->X;
	cpu_set_flag(c, c->Z, c->A == 0x00);
	cpu_set_flag(c, c->N, c->A & 0x80);
	return 0;
}

byte TXS(cpu *c) {
    c->P = c->X;
    return 0;
}

byte TYA(cpu *c) {
    c->A = c->Y;
	cpu_set_flag(c, c->Z, c->A == 0x00);
	cpu_set_flag(c, c->N, c->A & 0x80);
	return 0;
}

// ILLEGAL OPCODES USE AT YOUR OWN RISK
byte ALR(cpu *c) {
    UNUSED(c);
    return 0;
}

byte ANC(cpu *c) {
    UNUSED(c);
    return 0;
}

byte ANE(cpu *c) {
    UNUSED(c);
    return 0;
}

byte ARR(cpu *c) {
    UNUSED(c);
    return 0;
}

byte DCP(cpu *c) {
    UNUSED(c);
    return 0;
}

byte ISC(cpu *c) {
    UNUSED(c);
    return 0;
}

byte LAS(cpu *c) {
    UNUSED(c);
    return 0;
}

byte LAX(cpu *c) {
    UNUSED(c);
    return 0;
}

byte LXA(cpu *c) {
    UNUSED(c);
    return 0;
}

byte RLA(cpu *c) {
    UNUSED(c);
    return 0;
}

byte RRA(cpu *c) {
    UNUSED(c);
    return 0;
}

byte SAX(cpu *c) {
    UNUSED(c);
    return 0;
}

byte SBX(cpu *c) {
    UNUSED(c);
    return 0;
}

byte SHA(cpu *c) {
    UNUSED(c);
    return 0;
}

byte SHX(cpu *c) {
    UNUSED(c);
    return 0;
}

byte SHY(cpu *c) {
    UNUSED(c);
    return 0;
}

byte SLO(cpu *c) {
    UNUSED(c);
    return 0;
}

byte SRE(cpu *c) {
    UNUSED(c);
    return 0;
}

byte TAS(cpu *c) {
    UNUSED(c);
    return 0;
}

byte USBC(cpu *c) {
    UNUSED(c);
    return 0;
}

byte NOP_undoc(cpu *c) {
    UNUSED(c);
    return 0;
}

byte JAM (cpu *c) {
    UNUSED(c);
    return 0;
}


void cpu_code(cpu *c) {
    // System Instructions
    c->code[0x00] = (struct code_t){"BRK", &IMP, &BRK, 7};

    // Load/Store Operations
    c->code[0xA9] = (struct code_t){"LDA", &IMM, &LDA, 2}; // LDA Immediate
    c->code[0xA5] = (struct code_t){"LDA", &ZPG, &LDA, 3}; // LDA Zero Page
    c->code[0xB5] = (struct code_t){"LDA", &ZPX, &LDA, 4}; // LDA Zero Page,X
    c->code[0xAD] = (struct code_t){"LDA", &ABS, &LDA, 4}; // LDA Absolute
    c->code[0xBD] = (struct code_t){"LDA", &ABX, &LDA, 4}; // LDA Absolute,X
    c->code[0xB9] = (struct code_t){"LDA", &ABY, &LDA, 4}; // LDA Absolute,Y
    c->code[0xA1] = (struct code_t){"LDA", &IZX, &LDA, 6}; // LDA (Indirect,X)
    c->code[0xB1] = (struct code_t){"LDA", &IZY, &LDA, 5}; // LDA (Indirect),Y

    // LDX (Load X Register)
    c->code[0xA2] = (struct code_t){"LDX", &IMM, &LDX, 2};
    c->code[0xA6] = (struct code_t){"LDX", &ZPG, &LDX, 3};
    c->code[0xB6] = (struct code_t){"LDX", &ZPY, &LDX, 4};
    c->code[0xAE] = (struct code_t){"LDX", &ABS, &LDX, 4};
    c->code[0xBE] = (struct code_t){"LDX", &ABY, &LDX, 4};

    // LDY (Load Y Register)
    c->code[0xA0] = (struct code_t){"LDY", &IMM, &LDY, 2};
    c->code[0xA4] = (struct code_t){"LDY", &ZPG, &LDY, 3};
    c->code[0xB4] = (struct code_t){"LDY", &ZPX, &LDY, 4};
    c->code[0xAC] = (struct code_t){"LDY", &ABS, &LDY, 4};
    c->code[0xBC] = (struct code_t){"LDY", &ABX, &LDY, 4};

    // ADC Immediate
    c->code[0x69] = (struct code_t){"ADC", &IMM, &ADC, 2};
    // ADC Zero Page
    c->code[0x65] = (struct code_t){"ADC", &ZPG, &ADC, 3};
    // ADC Zero Page,X
    c->code[0x75] = (struct code_t){"ADC", &ZPX, &ADC, 4};
    // ADC Absolute
    c->code[0x6D] = (struct code_t){"ADC", &ABS, &ADC, 4};
    // ADC Absolute,X
    c->code[0x7D] = (struct code_t){"ADC", &ABX, &ADC, 4}; // Note: +1 if page boundary is crossed
    // ADC Absolute,Y
    c->code[0x79] = (struct code_t){"ADC", &ABY, &ADC, 4}; // Note: +1 if page boundary is crossed
    // ADC (Indirect,X)
    c->code[0x61] = (struct code_t){"ADC", &IZX, &ADC, 6};
    // ADC (Indirect),Y
    c->code[0x71] = (struct code_t){"ADC", &IZY, &ADC, 5}; // Note: +1 if page boundary is crossed

    // AND Immediate
    c->code[0x29] = (struct code_t){"AND", &IMM, &AND, 2};
    // AND Zero Page
    c->code[0x25] = (struct code_t){"AND", &ZPG, &AND, 3};
    // AND Zero Page,X
    c->code[0x35] = (struct code_t){"AND", &ZPX, &AND, 4};
    // AND Absolute
    c->code[0x2D] = (struct code_t){"AND", &ABS, &AND, 4};
    // AND Absolute,X
    c->code[0x3D] = (struct code_t){"AND", &ABX, &AND, 4}; // Note: +1 if page boundary is crossed
    // AND Absolute,Y
    c->code[0x39] = (struct code_t){"AND", &ABY, &AND, 4}; // Note: +1 if page boundary is crossed
    // AND (Indirect,X)
    c->code[0x21] = (struct code_t){"AND", &IZX, &AND, 6};
    // AND (Indirect),Y
    c->code[0x31] = (struct code_t){"AND", &IZY, &AND, 5}; // Note: +1 if page boundary is crossed

    // ASL with Accumulator
    c->code[0x0A] = (struct code_t){"ASL", &ACC, &ASL, 2}; // ASL Accumulator
    // ASL with Zero Page
    c->code[0x06] = (struct code_t){"ASL", &ZPG, &ASL, 5}; // ASL Zero Page
    // ASL with Zero Page,X
    c->code[0x16] = (struct code_t){"ASL", &ZPX, &ASL, 6}; // ASL Zero Page,X
    // ASL with Absolute
    c->code[0x0E] = (struct code_t){"ASL", &ABS, &ASL, 6}; // ASL Absolute
    // ASL with Absolute,X
    c->code[0x1E] = (struct code_t){"ASL", &ABX, &ASL, 7}; // ASL Absolute,X (Note: +1 cycle if page boundary is crossed)


    c->code[0x90] = (struct code_t){"BCC", &REL, &BCC, 2}; // +1 if branch succeeds, +2 if to a new page
    c->code[0xB0] = (struct code_t){"BCS", &REL, &BCS, 2}; // +1 if branch succeeds, +2 if to a new page
    c->code[0xF0] = (struct code_t){"BEQ", &REL, &BEQ, 2}; // +1 if branch succeeds, +2 if to a new page

    c->code[0x24] = (struct code_t){"BIT", &ZPG, &BIT, 3}; // BIT Zero Page
    c->code[0x2C] = (struct code_t){"BIT", &ABS, &BIT, 4}; // BIT Absolute

    c->code[0x30] = (struct code_t){"BMI", &REL, &BMI, 2}; // +1 if branch succeeds, +2 if to a new page

    c->code[0xD0] = (struct code_t){"BNE", &REL, &BNE, 2}; // +1 if branch succeeds, +2 if to a new page

    c->code[0x10] = (struct code_t){"BPL", &REL, &BPL, 2}; // +1 if branch succeeds, +2 if to a new page

    // BVC (Branch if Overflow Clear)
    c->code[0x50] = (struct code_t){"BVC", &REL, &BVC, 2}; // BVC Relative
    // BVS (Branch if Overflow Set)
    c->code[0x70] = (struct code_t){"BVS", &REL, &BVS, 2}; // BVS Relative

    // CLC (Clear Carry Flag)
    c->code[0x18] = (struct code_t){"CLC", &IMP, &CLC, 2};
    // CLD (Clear Decimal Mode)
    c->code[0xD8] = (struct code_t){"CLD", &IMP, &CLD, 2};
    // CLI (Clear Interrupt Disable)
    c->code[0x58] = (struct code_t){"CLI", &IMP, &CLI, 2};
    // CLV (Clear Overflow Flag)
    c->code[0xB8] = (struct code_t){"CLV", &IMP, &CLV, 2};

    // CMP (Compare Accumulator)
    c->code[0xC9] = (struct code_t){"CMP", &IMM, &CMP, 2};
    c->code[0xC5] = (struct code_t){"CMP", &ZPG, &CMP, 3};
    c->code[0xD5] = (struct code_t){"CMP", &ZPX, &CMP, 4};
    c->code[0xCD] = (struct code_t){"CMP", &ABS, &CMP, 4};
    c->code[0xDD] = (struct code_t){"CMP", &ABX, &CMP, 4};
    c->code[0xD9] = (struct code_t){"CMP", &ABY, &CMP, 4};
    c->code[0xC1] = (struct code_t){"CMP", &IZX, &CMP, 6};
    c->code[0xD1] = (struct code_t){"CMP", &IZY, &CMP, 5};

    // CPX (Compare X Register)
    c->code[0xE0] = (struct code_t){"CPX", &IMM, &CPX, 2};
    c->code[0xE4] = (struct code_t){"CPX", &ZPG, &CPX, 3};
    c->code[0xEC] = (struct code_t){"CPX", &ABS, &CPX, 4};

    // CPY (Compare Y Register)
    c->code[0xC0] = (struct code_t){"CPY", &IMM, &CPY, 2};
    c->code[0xC4] = (struct code_t){"CPY", &ZPG, &CPY, 3};
    c->code[0xCC] = (struct code_t){"CPY", &ABS, &CPY, 4};

    // DEC (Decrement Memory)
    c->code[0xC6] = (struct code_t){"DEC", &ZPG, &DEC, 5};
    c->code[0xD6] = (struct code_t){"DEC", &ZPX, &DEC, 6};
    c->code[0xCE] = (struct code_t){"DEC", &ABS, &DEC, 6};
    c->code[0xDE] = (struct code_t){"DEC", &ABX, &DEC, 7};

    // DEX (Decrement X Register)
    c->code[0xCA] = (struct code_t){"DEX", &IMP, &DEX, 2};

    // DEY (Decrement Y Register)
    c->code[0x88] = (struct code_t){"DEY", &IMP, &DEY, 2};

    // EOR (Exclusive OR)
    c->code[0x49] = (struct code_t){"EOR", &IMM, &EOR, 2};
    c->code[0x45] = (struct code_t){"EOR", &ZPG, &EOR, 3};
    c->code[0x55] = (struct code_t){"EOR", &ZPX, &EOR, 4};
    c->code[0x4D] = (struct code_t){"EOR", &ABS, &EOR, 4};
    c->code[0x5D] = (struct code_t){"EOR", &ABX, &EOR, 4};
    c->code[0x59] = (struct code_t){"EOR", &ABY, &EOR, 4};
    c->code[0x41] = (struct code_t){"EOR", &IZX, &EOR, 6};
    c->code[0x51] = (struct code_t){"EOR", &IZY, &EOR, 5};

    // INC (Increment Memory)
    c->code[0xE6] = (struct code_t){"INC", &ZPG, &INC, 5};
    c->code[0xF6] = (struct code_t){"INC", &ZPX, &INC, 6};
    c->code[0xEE] = (struct code_t){"INC", &ABS, &INC, 6};
    c->code[0xFE] = (struct code_t){"INC", &ABX, &INC, 7};

    // INX (Increment X Register)
    c->code[0xE8] = (struct code_t){"INX", &IMP, &INX, 2};

    // INY (Increment Y Register)
    c->code[0xC8] = (struct code_t){"INY", &IMP, &INY, 2};

    // JMP (Jump)
    c->code[0x4C] = (struct code_t){"JMP", &ABS, &JMP, 3}; // Absolute jump
    c->code[0x6C] = (struct code_t){"JMP", &IND, &JMP, 5}; // Indirect jump

    // JSR (Jump to Subroutine)
    c->code[0x20] = (struct code_t){"JSR", &ABS, &JSR, 6};


    // LSR (Logical Shift Right)
    c->code[0x4A] = (struct code_t){"LSR", &ACC, &LSR, 2}; // Accumulator
    c->code[0x46] = (struct code_t){"LSR", &ZPG, &LSR, 5}; // Zero Page
    c->code[0x56] = (struct code_t){"LSR", &ZPX, &LSR, 6}; // Zero Page,X
    c->code[0x4E] = (struct code_t){"LSR", &ABS, &LSR, 6}; // Absolute
    c->code[0x5E] = (struct code_t){"LSR", &ABX, &LSR, 7}; // Absolute,X

    // NOP (No Operation)
    c->code[0xEA] = (struct code_t){"NOP", &IMP, &NOP, 2};

    // ORA (Logical Inclusive OR)
    c->code[0x09] = (struct code_t){"ORA", &IMM, &ORA, 2};
    c->code[0x05] = (struct code_t){"ORA", &ZPG, &ORA, 3};
    c->code[0x15] = (struct code_t){"ORA", &ZPX, &ORA, 4};
    c->code[0x0D] = (struct code_t){"ORA", &ABS, &ORA, 4};
    c->code[0x1D] = (struct code_t){"ORA", &ABX, &ORA, 4};
    c->code[0x19] = (struct code_t){"ORA", &ABY, &ORA, 4};
    c->code[0x01] = (struct code_t){"ORA", &IZX, &ORA, 6};
    c->code[0x11] = (struct code_t){"ORA", &IZY, &ORA, 5};

    // PHA (Push Accumulator)
    c->code[0x48] = (struct code_t){"PHA", &IMP, &PHA, 3};

    // PHP (Push Processor Status)
    c->code[0x08] = (struct code_t){"PHP", &IMP, &PHP, 3};

    // PLA (Pull Accumulator)
    c->code[0x68] = (struct code_t){"PLA", &IMP, &PLA, 4};

    // PLP (Pull Processor Status)
    c->code[0x28] = (struct code_t){"PLP", &IMP, &PLP, 4};

    // ROL (Rotate Left)
    c->code[0x2A] = (struct code_t){"ROL", &ACC, &ROL, 2}; // Accumulator
    c->code[0x26] = (struct code_t){"ROL", &ZPG, &ROL, 5}; // Zero Page
    c->code[0x36] = (struct code_t){"ROL", &ZPX, &ROL, 6}; // Zero Page,X
    c->code[0x2E] = (struct code_t){"ROL", &ABS, &ROL, 6}; // Absolute
    c->code[0x3E] = (struct code_t){"ROL", &ABX, &ROL, 7}; // Absolute,X

    // ROR (Rotate Right)
    c->code[0x6A] = (struct code_t){"ROR", &ACC, &ROR, 2}; // Accumulator
    c->code[0x66] = (struct code_t){"ROR", &ZPG, &ROR, 5}; // Zero Page
    c->code[0x76] = (struct code_t){"ROR", &ZPX, &ROR, 6}; // Zero Page,X
    c->code[0x6E] = (struct code_t){"ROR", &ABS, &ROR, 6}; // Absolute
    c->code[0x7E] = (struct code_t){"ROR", &ABX, &ROR, 7}; // Absolute,X

    // RTI (Return from Interrupt)
    c->code[0x40] = (struct code_t){"RTI", &IMP, &RTI, 6};

    // RTS (Return from Subroutine)
    c->code[0x60] = (struct code_t){"RTS", &IMP, &RTS, 6};

    // SBC (Subtract with Carry)
    c->code[0xE9] = (struct code_t){"SBC", &IMM, &SBC, 2}; // Immediate
    c->code[0xE5] = (struct code_t){"SBC", &ZPG, &SBC, 3}; // Zero Page
    c->code[0xF5] = (struct code_t){"SBC", &ZPX, &SBC, 4}; // Zero Page,X
    c->code[0xED] = (struct code_t){"SBC", &ABS, &SBC, 4}; // Absolute
    c->code[0xFD] = (struct code_t){"SBC", &ABX, &SBC, 4}; // Absolute,X
    c->code[0xF9] = (struct code_t){"SBC", &ABY, &SBC, 4}; // Absolute,Y
    c->code[0xE1] = (struct code_t){"SBC", &IZX, &SBC, 6}; // (Indirect,X)
    c->code[0xF1] = (struct code_t){"SBC", &IZY, &SBC, 5}; // (Indirect),Y

    // SEC (Set Carry Flag)
    c->code[0x38] = (struct code_t){"SEC", &IMP, &SEC, 2};

    // SED (Set Decimal Flag)
    c->code[0xF8] = (struct code_t){"SED", &IMP, &SED, 2};

    // SEI (Set Interrupt Disable)
    c->code[0x78] = (struct code_t){"SEI", &IMP, &SEI, 2};

    // STA (Store Accumulator)
    c->code[0x85] = (struct code_t){"STA", &ZPG, &STA, 3}; // Zero Page
    c->code[0x95] = (struct code_t){"STA", &ZPX, &STA, 4}; // Zero Page,X
    c->code[0x8D] = (struct code_t){"STA", &ABS, &STA, 4}; // Absolute
    c->code[0x9D] = (struct code_t){"STA", &ABX, &STA, 5}; // Absolute,X
    c->code[0x99] = (struct code_t){"STA", &ABY, &STA, 5}; // Absolute,Y
    c->code[0x81] = (struct code_t){"STA", &IZX, &STA, 6}; // (Indirect,X)
    c->code[0x91] = (struct code_t){"STA", &IZY, &STA, 6}; // (Indirect),Y

    // STX (Store X Register)
    c->code[0x86] = (struct code_t){"STX", &ZPG, &STX, 3}; // Zero Page
    c->code[0x96] = (struct code_t){"STX", &ZPY, &STX, 4}; // Zero Page,Y
    c->code[0x8E] = (struct code_t){"STX", &ABS, &STX, 4}; // Absolute

    // STY (Store Y Register)
    c->code[0x84] = (struct code_t){"STY", &ZPG, &STY, 3}; // Zero Page
    c->code[0x94] = (struct code_t){"STY", &ZPX, &STY, 4}; // Zero Page,X
    c->code[0x8C] = (struct code_t){"STY", &ABS, &STY, 4}; // Absolute

    // TAX (Transfer Accumulator to X)
    c->code[0xAA] = (struct code_t){"TAX", &IMP, &TAX, 2};

    // TAY (Transfer Accumulator to Y)
    c->code[0xA8] = (struct code_t){"TAY", &IMP, &TAY, 2};

    // TSX (Transfer Stack Pointer to X)
    c->code[0xBA] = (struct code_t){"TSX", &IMP, &TSX, 2};

    // TXA (Transfer X to Accumulator)
    c->code[0x8A] = (struct code_t){"TXA", &IMP, &TXA, 2};

    // TXS (Transfer X to Stack Pointer)
    c->code[0x9A] = (struct code_t){"TXS", &IMP, &TXS, 2};

    // TYA (Transfer Y to Accumulator)
    c->code[0x98] = (struct code_t){"TYA", &IMP, &TYA, 2};

    // ILLEGAL OPCODES

    // ALR - AND byte with accumulator, then LSR A
    c->code[0x4B] = (struct code_t){"ALR", &IMM, &ALR, 2};

    // ANC - AND byte with accumulator, then copy bit 7 of A into C
    c->code[0x0B] = (struct code_t){"ANC", &IMM, &ANC, 2}; // Also ANC can have another opcode 0x2B, depending on the variant
    c->code[0x2B] = (struct code_t){"ANC", &IMM, &ANC, 2};

    // ANE - AND X register with accumulator and an immediate value, then store the result in A (unstable)
    c->code[0x8B] = (struct code_t){"ANE", &IMM, &ANE, 2};

    // ARR - AND byte with accumulator, then rotate one bit right in the accumulator, and check bit 5 and 6 to set flags
    c->code[0x6B] = (struct code_t){"ARR", &IMM, &ARR, 2};

    // DCP - Decrement memory by one, then compare memory with accumulator
    c->code[0xC7] = (struct code_t){"DCP", &ZPG, &DCP, 5};
    c->code[0xD7] = (struct code_t){"DCP", &ZPX, &DCP, 6};
    c->code[0xCF] = (struct code_t){"DCP", &ABS, &DCP, 6};
    c->code[0xDF] = (struct code_t){"DCP", &ABX, &DCP, 7};
    c->code[0xDB] = (struct code_t){"DCP", &ABY, &DCP, 7};
    c->code[0xC3] = (struct code_t){"DCP", &IZX, &DCP, 8};
    c->code[0xD3] = (struct code_t){"DCP", &IZY, &DCP, 8};

    // ISC - Increase memory by one, then subtract memory from accumulator (with carry)
    c->code[0xE7] = (struct code_t){"ISC", &ZPG, &ISC, 5};
    c->code[0xF7] = (struct code_t){"ISC", &ZPX, &ISC, 6};
    c->code[0xEF] = (struct code_t){"ISC", &ABS, &ISC, 6};
    c->code[0xFF] = (struct code_t){"ISC", &ABX, &ISC, 7};
    c->code[0xFB] = (struct code_t){"ISC", &ABY, &ISC, 7};
    c->code[0xE3] = (struct code_t){"ISC", &IZX, &ISC, 8};
    c->code[0xF3] = (struct code_t){"ISC", &IZY, &ISC, 8};

    // LAS - Load accumulator and stack pointer with AND of stack pointer and memory
    c->code[0xBB] = (struct code_t){"LAS", &ABY, &LAS, 4};

    // LAX - Load accumulator and X register with memory
    c->code[0xA7] = (struct code_t){"LAX", &ZPG, &LAX, 3};
    c->code[0xB7] = (struct code_t){"LAX", &ZPY, &LAX, 4};
    c->code[0xAF] = (struct code_t){"LAX", &ABS, &LAX, 4};
    c->code[0xBF] = (struct code_t){"LAX", &ABY, &LAX, 4};
    c->code[0xA3] = (struct code_t){"LAX", &IZX, &LAX, 6};
    c->code[0xB3] = (struct code_t){"LAX", &IZY, &LAX, 5};

    // LXA - Illegal opcode, behaves similarly to LAX
    c->code[0xAB] = (struct code_t){"LXA", &IMM, &LXA, 2};

    // RLA - Rotate one bit left in memory, then AND accumulator with memory
    c->code[0x27] = (struct code_t){"RLA", &ZPG, &RLA, 5};
    c->code[0x37] = (struct code_t){"RLA", &ZPX, &RLA, 6};
    c->code[0x2F] = (struct code_t){"RLA", &ABS, &RLA, 6};
    c->code[0x3F] = (struct code_t){"RLA", &ABX, &RLA, 7};
    c->code[0x3B] = (struct code_t){"RLA", &ABY, &RLA, 7};
    c->code[0x23] = (struct code_t){"RLA", &IZX, &RLA, 8};
    c->code[0x33] = (struct code_t){"RLA", &IZY, &RLA, 8};

    // RRA - Rotate one bit right in memory, then add memory to accumulator with carry
    c->code[0x67] = (struct code_t){"RRA", &ZPG, &RRA, 5};
    c->code[0x77] = (struct code_t){"RRA", &ZPX, &RRA, 6};
    c->code[0x6F] = (struct code_t){"RRA", &ABS, &RRA, 6};
    c->code[0x7F] = (struct code_t){"RRA", &ABX, &RRA, 7};
    c->code[0x7B] = (struct code_t){"RRA", &ABY, &RRA, 7};
    c->code[0x63] = (struct code_t){"RRA", &IZX, &RRA, 8};
    c->code[0x73] = (struct code_t){"RRA", &IZY, &RRA, 8};

    // SAX - Store A AND X
    c->code[0x87] = (struct code_t){"SAX", &ZPG, &SAX, 3};
    c->code[0x97] = (struct code_t){"SAX", &ZPY, &SAX, 4};
    c->code[0x8F] = (struct code_t){"SAX", &ABS, &SAX, 4};
    c->code[0x83] = (struct code_t){"SAX", &IZX, &SAX, 6};

    // SBX - Subtract memory from A and X (AND X with A, then subtract memory from result)
    c->code[0xCB] = (struct code_t){"SBX", &IMM, &SBX, 2};

    // SHA - Store A AND X AND the high byte of the target address plus one
    c->code[0x9F] = (struct code_t){"SHA", &ABY, &SHA, 5};
    c->code[0x93] = (struct code_t){"SHA", &IZY, &SHA, 6};

    // SHX - Store X AND the high byte of the target address plus one
    c->code[0x9E] = (struct code_t){"SHX", &ABY, &SHX, 5};

    // SHY - Store Y AND the high byte of the target address plus one
    c->code[0x9C] = (struct code_t){"SHY", &ABX, &SHY, 5};

    // SLO - Shift Left then OR (Unofficial opcode)
    c->code[0x07] = (struct code_t){"SLO", &ZPG, &SLO, 5};
    c->code[0x17] = (struct code_t){"SLO", &ZPX, &SLO, 6};
    c->code[0x0F] = (struct code_t){"SLO", &ABS, &SLO, 6};
    c->code[0x1F] = (struct code_t){"SLO", &ABX, &SLO, 7};
    c->code[0x1B] = (struct code_t){"SLO", &ABY, &SLO, 7};
    c->code[0x03] = (struct code_t){"SLO", &IZX, &SLO, 8};
    c->code[0x13] = (struct code_t){"SLO", &IZY, &SLO, 8};

    // SRE - Shift Right then Exclusive OR (Unofficial opcode)
    c->code[0x47] = (struct code_t){"SRE", &ZPG, &SRE, 5};
    c->code[0x57] = (struct code_t){"SRE", &ZPX, &SRE, 6};
    c->code[0x4F] = (struct code_t){"SRE", &ABS, &SRE, 6};
    c->code[0x5F] = (struct code_t){"SRE", &ABX, &SRE, 7};
    c->code[0x5B] = (struct code_t){"SRE", &ABY, &SRE, 7};
    c->code[0x43] = (struct code_t){"SRE", &IZX, &SRE, 8};
    c->code[0x53] = (struct code_t){"SRE", &IZY, &SRE, 8};

    // TAS - AND X register with A and store result in stack pointer, then AND stack pointer with the high byte of the target address of the argument + 1. Store the result in memory. (Unofficial opcode)
    c->code[0x9B] = (struct code_t){"TAS", &ABY, &TAS, 5};

    // USBC - Unofficial opcode, often acts like SBC (Subtract with Carry)
    c->code[0xEB] = (struct code_t){"SBC", &IMM, &USBC, 2}; // Example using IMM addressing, actual behavior may vary

    // NOP_undoc - Undocumented No Operation variants with different cycles or addressing modes
// NOPs (No Operation) - Implied
    c->code[0x1A] = (struct code_t){"NOP", &IMP, &NOP_undoc, 2};
    c->code[0x3A] = (struct code_t){"NOP", &IMP, &NOP_undoc, 2};
    c->code[0x5A] = (struct code_t){"NOP", &IMP, &NOP_undoc, 2};
    c->code[0x7A] = (struct code_t){"NOP", &IMP, &NOP_undoc, 2};
    c->code[0xDA] = (struct code_t){"NOP", &IMP, &NOP_undoc, 2};
    c->code[0xFA] = (struct code_t){"NOP", &IMP, &NOP_undoc, 2};

    // NOPs (No Operation) - Immediate
    c->code[0x80] = (struct code_t){"NOP", &IMM, &NOP_undoc, 2};
    c->code[0x82] = (struct code_t){"NOP", &IMM, &NOP_undoc, 2};
    c->code[0x89] = (struct code_t){"NOP", &IMM, &NOP_undoc, 2};
    c->code[0xC2] = (struct code_t){"NOP", &IMM, &NOP_undoc, 2};
    c->code[0xE2] = (struct code_t){"NOP", &IMM, &NOP_undoc, 2};

    // NOPs (No Operation) - Zero Page
    c->code[0x04] = (struct code_t){"NOP", &ZPG, &NOP_undoc, 3};
    c->code[0x44] = (struct code_t){"NOP", &ZPG, &NOP_undoc, 3};
    c->code[0x64] = (struct code_t){"NOP", &ZPG, &NOP_undoc, 3};

    // NOPs (No Operation) - Zero Page,X
    c->code[0x14] = (struct code_t){"NOP", &ZPX, &NOP_undoc, 4};
    c->code[0x34] = (struct code_t){"NOP", &ZPX, &NOP_undoc, 4};
    c->code[0x54] = (struct code_t){"NOP", &ZPX, &NOP_undoc, 4};
    c->code[0x74] = (struct code_t){"NOP", &ZPX, &NOP_undoc, 4};
    c->code[0xD4] = (struct code_t){"NOP", &ZPX, &NOP_undoc, 4};
    c->code[0xF4] = (struct code_t){"NOP", &ZPX, &NOP_undoc, 4};

    // NOPs (No Operation) - Absolute
    c->code[0x0C] = (struct code_t){"NOP", &ABS, &NOP_undoc, 4};

    // NOPs (No Operation) - Absolute,X
    c->code[0x1C] = (struct code_t){"NOP", &ABX, &NOP_undoc, 4}; // Page boundary crossed cycles may vary
    c->code[0x3C] = (struct code_t){"NOP", &ABX, &NOP_undoc, 4}; // Page boundary crossed cycles may vary
    c->code[0x5C] = (struct code_t){"NOP", &ABX, &NOP_undoc, 4}; // Page boundary crossed cycles may vary
    c->code[0x7C] = (struct code_t){"NOP", &ABX, &NOP_undoc, 4}; // Page boundary crossed cycles may vary
    c->code[0xDC] = (struct code_t){"NOP", &ABX, &NOP_undoc, 4}; // Page boundary crossed cycles may vary
    c->code[0xFC] = (struct code_t){"NOP", &ABX, &NOP_undoc, 4}; // Page boundary crossed cycles may vary

    // JAM (or KIL) - Causes the CPU to halt and do nothing until a reset occurs
    c->code[0x02] = (struct code_t){"JAM", &IMP, &JAM, 0}; // Cycle count is often irrelevant as the CPU halts
    c->code[0x12] = (struct code_t){"JAM", &IMP, &JAM, 0};
    c->code[0x22] = (struct code_t){"JAM", &IMP, &JAM, 0};
    c->code[0x32] = (struct code_t){"JAM", &IMP, &JAM, 0};
    c->code[0x42] = (struct code_t){"JAM", &IMP, &JAM, 0};
    c->code[0x52] = (struct code_t){"JAM", &IMP, &JAM, 0};
    c->code[0x62] = (struct code_t){"JAM", &IMP, &JAM, 0};
    c->code[0x72] = (struct code_t){"JAM", &IMP, &JAM, 0};
    c->code[0x92] = (struct code_t){"JAM", &IMP, &JAM, 0};
    c->code[0xB2] = (struct code_t){"JAM", &IMP, &JAM, 0};
    c->code[0xD2] = (struct code_t){"JAM", &IMP, &JAM, 0};
    c->code[0xF2] = (struct code_t){"JAM", &IMP, &JAM, 0};
}
