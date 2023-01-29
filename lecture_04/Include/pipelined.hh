#ifndef _PIPELINED_HH_
#define _PIPELINED_HH_

#include<stdlib.h>
#include<stdint.h>
#include<assert.h>
#include<vector>

namespace pipelined
{
    typedef uint8_t	u8;
    typedef uint32_t	u32;
    typedef uint64_t	u64;

    typedef int16_t	i16;

    typedef float	fp32;
    typedef double	fp64;

    namespace params
    {
	namespace GPR
	{
	    extern const u32	N;
	};

	namespace L1
	{
	    extern const u32	nsets;
	    extern const u32	nways;
	    extern const u32	linesize;
	    extern const u32	latency;
	};

	namespace MEM
	{
	    extern const u32	N;
	    extern const u32	latency;
	};
    };

    namespace counters
    {
	extern u64	instructions;
	extern u64	cycles;

	namespace L1
	{
	    extern u64	hits;
	    extern u64	misses;
	    extern u64	accesses;
	};
    };

    typedef enum
    {
        r0  =  0,
        r1  =  1,
        r2  =  2,
        r3  =  3,
        r4  =  4,
        r5  =  5,
        r6  =  6,
        r7  =  7,
        r8  =  8,
        r9  =  9,
	r10 = 10,
	r11 = 11,
	r12 = 12,
	r13 = 13,
	r14 = 14,
	r15 = 15
    } gprnum;                                   // valid GPR numbers

    typedef enum
    {
        f0 = 0,
        f1 = 1,
        f2 = 2,
        f3 = 3,
        f4 = 4,
        f5 = 5,
        f6 = 6,
        f7 = 7
    } fprnum;                                   // valid FPR numbers

    typedef struct
    {
        bool    LT;                             // less than
        bool    GT;                             // greater than
        bool    EQ;                             // equal to
    } flags_t;                                  // flags

    extern std::vector<u8>	MEM;
    extern std::vector<u32>	GPR;
    extern flags_t		flags;

    namespace caches
    {
        class entry                             // cache entry
        {
            public:
                bool            valid;          // is entry valid?
                uint32_t        addr;           // address of this entry
                uint64_t        touched;        // last time this entry was used
        };

        typedef std::vector<entry>      set;
        typedef std::vector<set>        array;

        class cache
        {
            private:
                uint32_t        _nsets;
                uint32_t        _nways;
                uint32_t        _linesize;
                array           _sets;

            public:
                cache(uint32_t nsets, uint32_t nways, uint32_t linesize);

                uint32_t        nsets() const;          // number of sets
                uint32_t        nways() const;          // number of ways
                uint32_t        linesize() const;       // in bytes
                uint32_t        capacity() const;       // in bytes
                array&          sets();                 // cache array
                bool            hit(uint32_t);          // tests for address hit in cache
                void            clear();                // clear the cache
        };

        extern cache L1;
    };

    extern uint32_t     CIA;                    // current instruction address
    extern uint32_t     NIA;                    // next instruction address

    void zeromem();
    void zeroctrs();

    namespace operations
    {
	class operation
	{
	    public:
		virtual bool execute() = 0;
	};

	bool process(operation* op);

	class addi : public operation
	{
	    private:
		gprnum	_RT;
		gprnum 	_RA;
		i16	_SI;
	    public:
		addi(gprnum RT, gprnum RA, i16 SI) { _RT = RT; _RA = RA, _SI = SI; }
		static void execute(gprnum RT, gprnum RA, i16 SI) { process(new addi(RT, RA, SI)); }
		bool execute() { GPR[_RT] = GPR[_RA] + _SI; return false; }
	};

	class cmpi : public operation
	{
	    private:
		gprnum	_RA;
		i16	_SI;
	    public:
		cmpi(gprnum RA, i16 SI) { _RA = RA; _SI = SI; }
		static void execute(gprnum RA, i16 SI) { process(new cmpi(RA, SI)); }
		bool execute() 
		{
		    flags.LT = false; flags.GT = false; flags.EQ = false;
		    if      (GPR[_RA] < _SI) flags.LT = true;
        	    else if (GPR[_RA] > _SI) flags.GT = true;
        	    else                     flags.EQ = true;

		    return false; 
		}	
	};

	class lbz : public operation
	{
	    private:
		gprnum	_RT;
		gprnum	_RA;
	    public:
		lbz(gprnum RT, gprnum RA) { _RT = RT; _RA = RA; }
		static void execute(gprnum RT, gprnum RA) { process(new lbz(RT, RA)); }
		bool execute() 
		{ 
		    uint32_t EA = GPR[_RA]; 
		    GPR[_RT] = MEM[EA];
		    return false; 
		}
	};

	class stb : public operation
	{
	    private:
		gprnum	_RS;
		gprnum	_RA;
	    public:
		stb(gprnum RS, gprnum RA) { _RS = RS; _RA = RA; }
		static void execute(gprnum RS, gprnum RA) { process(new stb(RS, RA)); }
		bool execute() 
		{
		    uint32_t EA = GPR[_RA];
                    MEM[EA] = GPR[_RS] & 0xff;

		    return false; 
		}
	};

	class b : public operation
	{
	    private:
		i16	_BD;
	    public:
		b(i16 BD) { _BD = BD; }
		static bool execute(i16 BD) { return process(new b(BD)); }
		bool execute() { NIA = CIA + _BD; return true; }
	};

	class beq : public operation
	{
	    private:
		i16	_BD;
	    public:
		beq(i16 BD) { _BD = BD; }
		static bool execute(i16 BD) { return process(new beq(BD)); }
		bool execute() { if (flags.EQ) { NIA = CIA + _BD; return true; } else return false; }
	};
    };

    namespace instructions
    {
	class instruction
	{
	};

	class nop : public instruction
	{
	};

	class addi : public instruction
	{
	    public:
		static void execute(gprnum RT, gprnum RA, i16 SI) { operations::addi::execute(RT, RA, SI); }
	};

	class cmpi : public instruction
	{
	    public:
		static void execute(gprnum RA, i16 SI) { operations::cmpi::execute(RA, SI); }
	};

	class lbz : public instruction
	{
	    public:
		static void execute(gprnum RT, gprnum RA) { operations::lbz::execute(RT, RA); }
	};

	class stb : public instruction
	{
	    public:
		static void execute(gprnum RS, gprnum RA) { operations::stb::execute(RS, RA); }
	};

	class beq : public instruction
	{
	    public:
		static bool execute(i16 BD) { return operations::beq::execute(BD); }
	};

	class b : public instruction
	{
	    public :
		static bool execute(i16 BD) { return operations::b::execute(BD); }
	};
    };
};

#endif
