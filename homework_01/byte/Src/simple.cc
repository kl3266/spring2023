#include<stdlib.h>
#include<stdint.h>
#include<simple.hh>

namespace simple
{
    const uint32_t 	N = 65536;		// 64 KiB memory
    uint8_t     	MEM[N];     		// memory is an array of N bytes
    uint32_t   	 	GPR[8];     		// 8 x 32-bit general purpose registers

    const uint32_t	latencies::MEM = 300;	// main memory latency

    flags_t	flags;				// flags

    uint32_t    CIA; 				// current instruction address
    uint32_t    NIA;    			// next instruction address

    uint64_t	instructions = 0;		// instruction counter
    uint64_t	cycles = 0;			// cycle counter

    void zeromem()
    {
	for (uint32_t i=0; i<N; i++) MEM[i] = 0;
    }

    void zeroctrs()
    {
	instructions = 0;
	cycles = 0;
    }

    void lhz(int RT, int RA)                	// load byte and zero-extend into a register
    {
    uint32_t temp = 0;
	uint32_t EA = GPR[RA];                          // need to have MEM[EA] and MEM[EA+1] into LSBs of GPR[RT]
    temp = MEM[EA];
    temp = ((temp << 8) & 0xFFFFF00) ^ (MEM[EA+1] & 0x000000FF);                                 // shift a byte to the left & change LSB to 0 by masking (XOR or +?)
    GPR[RT] = temp;

	instructions++;
	cycles += latencies::MEM;
    }

    void sth(int RS, int RA)                	// store byte from register
    {
    uint32_t temp = 0;
	uint32_t EA = GPR[RA];
    //temp = GPR[RS] & 0x0000FFFF;
    //MEM[EA] = (temp & 0x0000FF00) >> 8;
    //MEM[EA+1] = (temp & 000000FF);
    temp = GPR[RS] & 0x0000FF00;
    temp >>= 8;
    MEM[EA] = temp;
    temp = GPR[RS] & 0x000000FF;
    MEM[EA+1] = temp;

	instructions++;
	cycles += latencies::MEM;
    }

    void lbz(int RT, int RA)                	// load byte and zero-extend into a register
    {
	uint32_t EA = GPR[RA];
	GPR[RT] = MEM[EA];

	instructions++;
	cycles += latencies::MEM;
    }

    void stb(int RS, int RA)                	// store byte from register
    {
	uint32_t EA = GPR[RA];
	MEM[EA] = GPR[RS] & 0xff;

	instructions++;
	cycles += latencies::MEM;
    }

    void cmpi(int RA, int16_t SI)           	// compare the contents of a register with a signed integer
    {
	flags.LT = false; flags.GT = false; flags.EQ = false;
	if      (GPR[RA] < SI) flags.LT = true;
	else if (GPR[RA] > SI) flags.GT = true;
	else   		       flags.EQ = true;

	instructions++;
	cycles++;
    }

    void addi(int RT, int RA, int16_t SI)   	// add the contents of a register to a signed integer
    {
	GPR[RT] = GPR[RA] + SI;

	instructions++;
	cycles++;
    }

#undef beq
    bool beq(int16_t BD)                    	// branch if comparison resuts was "equal"
    {
	if (flags.EQ) { NIA = CIA + BD; return true; }
	return false;
    }

#undef b
    bool b(int16_t BD)                      	// unconditional branch
    {
	NIA = CIA + BD;
	return true;
    }
};