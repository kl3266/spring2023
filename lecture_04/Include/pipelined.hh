#ifndef _PIPELINED_HH_
#define _PIPELINED_HH_

#include<stdlib.h>
#include<stdint.h>
#include<assert.h>
#include<vector>
#include<algorithm>

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

	namespace FPR
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
	extern u64	operations;
	extern u64	cycles;
	extern u64	lastissued;

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

    template<typename T> class reg
    {
	private:
	    T	_data;
	    u64	_ready;
	public:
	    reg() 			{ _data = 0; _ready = 0; }
	    const T& data() const 	{ return _data; }
	    T& data() 			{ return _data;}
	    const u64& ready() const 	{ return _ready; }
	    u64& ready()		{ return _ready; }
    };

    extern std::vector<u8>		MEM;
    extern std::vector<reg<u32>>	GPR;
    extern std::vector<reg<double>>	FPR;
    extern flags_t			flags;

    namespace units
    {
	class unit
	{
	    private:
		u64	_ready;

	    public:
		unit() 				{ _ready = 0; }
	    	const u64& ready() const 	{ return _ready; }
	    	u64& ready()			{ return _ready; }
	};

	extern unit	LDU;	// load unit
	extern unit	STU;	// store unit
	extern unit	FXU;	// fixed-point unit
	extern unit	FPU;	// floating-point unit
	extern unit	BRU;	// branch unit
    };

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

    static u64 max(u64 a, u64 b)		{ return a >= b ? a : b; }
    static u64 max(u64 a, u64 b, u64 c) 	{ return max(a, max(b,c)); }
    static u64 max(u64 a, u64 b, u64 c, u64 d)	{ return max(a, max(b, c, d)); }

    namespace operations
    {
	class operation
	{
	    public:
		virtual bool 		execute() = 0;
		virtual units::unit& 	unit() = 0;
		virtual void 		targetready(u64 cycle) = 0;
		virtual u32  		latency() 	{ return 1; }
		virtual u32  		throughput() 	{ return 1; }
		virtual u64	 	ready() = 0;
		virtual bool 		process()
		{
		    counters::operations++;					// increment operation count
		    u64 minissue = max(ready(), counters::lastissued + 1); 	// earliest time operation can issue based on dependences
		    u64 cycle = counters::cycles;				// current cycle count
		    u64 lat = latency();					// call latency just once, since it performs a cache access!
		    counters::cycles = std::max(cycle, minissue + lat); 	// current cycle could advance to the end of this operation
		    unit().ready() = minissue + throughput();			// unit will be ready again after inverse throughput cycles
		    targetready(minissue + lat);				// update ready time for output register
		    counters::lastissued = minissue;				// update time of last issue
		    return execute();
		}
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
		bool execute() 
		{ 
		    GPR[_RT].data() = GPR[_RA].data() + _SI; 
		    return false; 
		}
		units::unit& unit() { return units::FXU; }
		void targetready(u64 cycle) { GPR[_RT].ready() = cycle; }
		u64 ready() { return max(GPR[_RA].ready(), unit().ready()) + 1; }
	};

	class cmpi : public operation
	{
	    private:
		gprnum	_RA;
		i16	_SI;
	    public:
		cmpi(gprnum RA, i16 SI) { _RA = RA; _SI = SI; }
		bool execute() 
		{
		    flags.LT = false; flags.GT = false; flags.EQ = false;
		    if      (GPR[_RA].data() < _SI) flags.LT = true;
        	    else if (GPR[_RA].data() > _SI) flags.GT = true;
        	    else                            flags.EQ = true;

		    return false; 
		}	
		units::unit& unit() { return units::FXU; }
		void targetready(u64 cycle) { }
		u64 ready() { return max(GPR[_RA].ready(), unit().ready()) + 1; }
	};

	class lbz : public operation
	{
	    private:
		gprnum	_RT;
		gprnum	_RA;
	    public:
		lbz(gprnum RT, gprnum RA) { _RT = RT; _RA = RA; }
		bool execute() 
		{ 
		    u32 EA = GPR[_RA].data(); 
		    GPR[_RT].data() = MEM[EA];
		    return false; 
		}
		u32 latency() { u32 EA = GPR[_RA].data(); return caches::L1.hit(EA) ? params::L1::latency : params::MEM::latency; }
		units::unit& unit() { return units::LDU; }
		void targetready(u64 cycle) { GPR[_RT].ready() = cycle; }
		u64 ready() { return max(GPR[_RA].ready(), unit().ready()) + 1; }
	};

	class stb : public operation
	{
	    private:
		gprnum	_RS;
		gprnum	_RA;
	    public:
		stb(gprnum RS, gprnum RA) { _RS = RS; _RA = RA; }
		bool execute() 
		{
		    uint32_t EA = GPR[_RA].data();
                    MEM[EA] = GPR[_RS].data() & 0xff;

		    return false; 
		}
		u32 latency() { u32 EA = GPR[_RA].data(); return caches::L1.hit(EA) ? params::L1::latency : params::MEM::latency; }
		units::unit& unit() { return units::STU; }
		void targetready(u64 cycle) { }
		u64 ready() { return max(GPR[_RA].ready(), GPR[_RS].ready(), unit().ready()) + 1; }
	};

	class lfd : public operation
	{
	    private:
		fprnum	_FT;
		gprnum	_RA;
	    public:
		lfd(fprnum FT, gprnum RA) { _FT = FT; _RA = RA; }
		bool execute()
		{
		    u32 EA = GPR[_RA].data();
		    FPR[_FT].data() = *((double*)(MEM.data() + EA));
		    return false;
		}
		u32 latency() {  u32 EA = GPR[_RA].data(); return caches::L1.hit(EA) ? params::L1::latency : params::MEM::latency; }
		units::unit& unit() { return units::LDU; }
		void targetready(u64 cycle) { FPR[_FT].ready() = cycle; }
		u64 ready() { return max(GPR[_RA].ready(), unit().ready()) + 1; }
	};

	class stfd : public operation
	{
	    private:
		fprnum _FS;
		gprnum _RA;
	    public:
		stfd(fprnum FS, gprnum RA) { _FS = FS; _RA = RA; }
		bool execute()
		{
		    u32 EA = GPR[_RA].data();
		    *((double*)(MEM.data() + EA)) = FPR[_FS].data();
		    return false;
		}
		u32 latency() {  u32 EA = GPR[_RA].data(); return caches::L1.hit(EA) ? params::L1::latency : params::MEM::latency; }
		units::unit& unit() { return units::STU; }
		void targetready(u64 cycle) { }
		u64 ready() { return max(GPR[_RA].ready(), FPR[_FS].ready(), unit().ready()) + 1; }
	};

	class b : public operation
	{
	    private:
		i16	_BD;
	    public:
		b(i16 BD) { _BD = BD; }
		bool execute() { NIA = CIA + _BD; return true; }
		units::unit& unit() { return units::BRU; }
		void targetready(u64 cycle) { }
		u64 ready() { return 0; }
	};

	class beq : public operation
	{
	    private:
		i16	_BD;
	    public:
		beq(i16 BD) { _BD = BD; }
		bool execute() { if (flags.EQ) { NIA = CIA + _BD; return true; } else return false; }
		units::unit& unit() { return units::BRU; }
		void targetready(u64 cycle) { }
		u64 ready() { return 0; }
	};

	class zd : public operation
	{
	    private:
		fprnum	_FT;
	    public:
		zd(fprnum FT) { _FT = FT; }
		bool execute() { FPR[_FT].data() = 0.0; return false; }
		units::unit& unit() { return units::FPU; }
		void targetready(u64 cycle) { FPR[_FT].ready() = cycle; }
		u64 ready() { return 0; }
	};

	class fmul : public operation
	{
	    private:
		fprnum 	_FT;
		fprnum	_FA;
		fprnum	_FB;
	    public:
		fmul(fprnum FT, fprnum FA, fprnum FB) { _FT = FT; _FA = FA; _FB = FB; }
		bool execute() { FPR[_FT].data() = FPR[_FA].data() * FPR[_FB].data(); return false; }
		units::unit& unit() { return units::FPU; }
		void targetready(u64 cycle) { FPR[_FT].ready() = cycle; }
		u64 ready() { return max(FPR[_FA].ready(), FPR[_FB].ready(), unit().ready()) + 1; }
	};

	class fadd : public operation
	{
	    private:
		fprnum 	_FT;
		fprnum	_FA;
		fprnum	_FB;
	    public:
		fadd(fprnum FT, fprnum FA, fprnum FB) { _FT = FT; _FA = FA; _FB = FB; }
		bool execute() { FPR[_FT].data() = FPR[_FA].data() + FPR[_FB].data(); return false; }
		units::unit& unit() { return units::FPU; }
		void targetready(u64 cycle) { FPR[_FT].ready() = cycle; }
		u64 ready() { return max(FPR[_FA].ready(), FPR[_FB].ready(), unit().ready()) + 1; }
	};
    };

    namespace instructions
    {
	class instruction
	{
	    public:
		virtual bool process() { }
	};

	bool process(instruction* instr);

	class addi : public instruction
	{
	    private:
		gprnum	_RT;
		gprnum	_RA;
		i16	_SI;
	    public:
		addi(gprnum RT, gprnum RA, i16 SI) { _RT = RT; _RA = RA; _SI = SI; }
		bool process() { return operations::process(new operations::addi(_RT, _RA, _SI)); }
		static bool execute(gprnum RT, gprnum RA, i16 SI) { return instructions::process(new addi(RT, RA, SI)); }
	};

	class cmpi : public instruction
	{
	    private:
		gprnum	_RA;
		i16	_SI;
	    public:
		cmpi(gprnum RA, i16 SI) { _RA = RA; _SI = SI; }
		bool process() { return operations::process(new operations::cmpi(_RA, _SI)); }
		static bool execute(gprnum RA, i16 SI) { return instructions::process(new cmpi(RA, SI)); }
	};

	class lbz : public instruction
	{
	    private:
		gprnum 	_RT;
		gprnum	_RA;
	    public:
		lbz(gprnum RT, gprnum RA) { _RT = RT; _RA = RA; }
		bool process() { return operations::process(new operations::lbz(_RT, _RA)); }
		static bool execute(gprnum RT, gprnum RA) { return instructions::process(new lbz(RT, RA)); }
	};

	class stb : public instruction
	{
	    private:
		gprnum	_RS;
		gprnum	_RA;
	    public:
		stb(gprnum RS, gprnum RA) { _RS = RS, _RA = RA; }
		bool process() { return operations::process(new operations::stb(_RS, _RA)); }
		static bool execute(gprnum RS, gprnum RA) { return instructions::process(new stb(RS, RA)); }
	};

	class beq : public instruction
	{
	    private:
		i16	_BD;
	    public:
		beq(i16 BD) { _BD = BD; }
		bool process() { return operations::process(new operations::beq(_BD)); }
		static bool execute(i16 BD) { return instructions::process(new beq(BD)); }
	};

	class b : public instruction
	{
	    private:
		i16	_BD;
	    public:
		b(i16 BD) { _BD = BD; }
		bool process() { return operations::process(new operations::b(_BD)); }
		static bool execute(i16 BD) { return instructions::process(new b(BD)); }
	};

	class zd : public instruction
	{
	    private:
		fprnum	_FT;
	    public:
		zd(fprnum FT) { _FT = FT; }
		bool process() { return operations::process(new operations::zd(_FT)); }
		static bool execute(fprnum FT) { return instructions::process(new zd(FT)); }
	};

	class fmul : public instruction
	{
	    private:
		fprnum	_FT;
		fprnum	_FA;
		fprnum	_FB;
	    public:
		fmul(fprnum FT, fprnum FA, fprnum FB) { _FT = FT; _FA = FA; _FB = FB; }
		bool process() { return operations::process(new operations::fmul(_FT, _FA, _FB)); }
		static bool execute(fprnum FT, fprnum FA, fprnum FB) { return instructions::process(new fmul(FT, FA, FB)); }
	};


	class fadd : public instruction
	{
	    private:
		fprnum	_FT;
		fprnum	_FA;
		fprnum	_FB;
	    public:
		fadd(fprnum FT, fprnum FA, fprnum FB) { _FT = FT; _FA = FA; _FB = FB; }
		bool process() { return operations::process(new operations::fadd(_FT, _FA, _FB)); }
		static bool execute(fprnum FT, fprnum FA, fprnum FB) { return instructions::process(new fadd(FT, FA, FB)); }
	};

	class lfd : public instruction
	{
	    private:
		fprnum 	_FT;
		gprnum	_RA;
	    public:
		lfd(fprnum FT, gprnum RA) { _FT = FT; _RA = RA; }
		bool process() { return operations::process(new operations::lfd(_FT, _RA)); }
		static bool execute(fprnum FT, gprnum RA) { return instructions::process(new lfd(FT, RA)); }
	};

	class stfd : public instruction
	{
	    private:
		fprnum 	_FS;
		gprnum	_RA;
	    public:
		stfd(fprnum FS, gprnum RA) { _FS = FS; _RA = RA; }
		bool process() { return operations::process(new operations::stfd(_FS, _RA)); }
		static bool execute(fprnum FS, gprnum RA) { return instructions::process(new stfd(FS, RA)); }
	};
    };
};

#endif
