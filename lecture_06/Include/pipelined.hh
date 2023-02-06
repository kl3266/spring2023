#ifndef _PIPELINED_HH_
#define _PIPELINED_HH_

#include<stdlib.h>
#include<stdint.h>
#include<assert.h>
#include<vector>
#include<set>
#include<algorithm>
#include<iostream>
#include<iomanip>
#include<string>

namespace pipelined
{
    typedef uint8_t	u8;
    typedef uint32_t	u32;
    typedef uint64_t	u64;

    typedef int16_t	i16;

    typedef float	fp32;
    typedef double	fp64;

    extern bool tracing;

    namespace params
    {
	namespace PRF
	{
	    extern const u32	N;
	};

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

	namespace Backend
	{
	    extern const u32	maxissue;	// maximum operations that can be issued per cycle
	};

    };

    namespace counters
    {
	extern u64	instructions;
	extern u64	operations;
	extern u64	cycles;
	extern u64	lastissued;
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

    template<typename T> class preg		// a physical register
    {
	private:
	    T		_data;
	    u64		_ready;
	    bool	_busy;
	public:
	    preg() 			{ _data = 0; _ready = 0; _busy = false; }
	    const T& data() const 	{ return _data; }
	    T& data() 			{ return _data;}
	    const u64& ready() const 	{ return _ready; }
	    u64& ready()		{ return _ready; }
	    size_t size() const		{ return sizeof(T); }
	    bool& busy()		{ return _busy; }
	    const bool& busy() const	{ return _busy; }
    };

    namespace PRF
    {
	extern std::vector<preg<u64>>	R;	// physical register file (for all registers)
	extern u32			next;	// next physical register to use				
    };

    template<typename T> class reg		// an architected register
    {
	private:
	    u32 _idx;			// index into physical register file
	public:
	    reg() 			{ assert(sizeof(T) <= PRF::R[0].size()); assert(PRF::next < params::PRF::N); _idx = PRF::next++; }
	    const T& data() const 	{ return *((T*)(&(PRF::R[_idx].data()))); }
	    T& data() 			{ return *((T*)(&(PRF::R[_idx].data()))); }
	    const u64& ready() const 	{ return PRF::R[_idx].ready(); }
	    u64& ready()		{ return PRF::R[_idx].ready(); }
	    const bool& busy() const 	{ return PRF::R[_idx].busy(); }
	    bool& busy()		{ return PRF::R[_idx].busy(); }
	    const u32& idx() const	{ return _idx; }
	    u32& idx()			{ return _idx; }
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
		std::set<u64>	_busy;

	    public:
		unit() 					{ }
		void clear()				{ _busy.clear(); }
		const bool busy(u64 cycle) const	{ return _busy.count(cycle); }
		void claim(u64 cycle) 			{ _busy.insert(cycle); }
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
		std::vector<u8>	data;		// data in this entry

		entry(u32 linesize) : data(linesize) { }	// creates a cache entry with linesize bytes of storage
		entry()	{ }					// default constructor, to be filled later
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
		u64		accesses;					// counter of number of accesses
		u64		misses;						// counter of number of misses
		u64		hits;						// counter of number of hits

                cache(uint32_t nsets, uint32_t nways, uint32_t linesize);	// construct a cache of size nsets x nways x linesize bytes

                uint32_t        nsets() const;       			   	// number of sets
                uint32_t        nways() const;          			// number of ways
                uint32_t        linesize() const;    			   	// in bytes
                uint32_t        capacity() const;     			  	// in bytes
                array&          sets();                			 	// cache array
                bool            hit(u32 EA);	        			// tests for address EA hit in cache
		bool		contains(u32 EA, u32 L);			// tests if cache contains data in address range [EA, EA+L)
		bool		contains(u32 WA, u32 L, u32 &set, u32 &way);	// returns the set and way that contain the data (if true)
		u8*		fill(u32 EA, u32 L);				// loads data in address range [EA, EA+L) into cache, returns a pointer to the data in cache
                void            clear();                			// clear the cache
        };

        extern cache L1;
    };

    extern uint32_t     CIA;                    // current instruction address
    extern uint32_t     NIA;                    // next instruction address

    void zeromem();
    void zeroctrs();

    static u64 max(u64 a)			{ return a; }
    static u64 max(u64 a, u64 b)		{ return a >= b ? a : b; }
    static u64 max(u64 a, u64 b, u64 c) 	{ return max(a, max(b,c)); }
    static u64 max(u64 a, u64 b, u64 c, u64 d)	{ return max(a, max(b, c, d)); }

    namespace operations
    {
	extern std::multiset<u64>	issued;

	class operation
	{
	    private:
		static bool	first;	// first operation processed

		u64	_count;		// opearation #
		u64	_ready;		// inputs ready
		u64	_issue;		// issue time
		u64	_complete;	// completion time (output ready)
	    public:
		static	void 		zero() { first = true; }		// starting a new stream
		virtual units::unit& 	unit() = 0;				// functional unit for this operation
		virtual void 		target(u64 cycle) = 0;			// update ready time of output
		virtual u32  		latency() 	{ return 1; }		// operation latency
		virtual u32  		throughput() 	{ return 1; }		// operation throughput
		virtual u64	 	ready() = 0;				// time inputs are ready
		virtual std::string	dasm()  = 0;				// disassembly of the operation
		virtual bool		issue(u64 cycle) = 0; 			// issue operation at the cycle
		void output(std::ostream& out)
		{
		    if (first)
		    {
			first = false;
		    }

		    std::ios state(nullptr);
		    state.copyfmt(out);
		    out << std::setw( 9) << std::setfill('0') << _count     << " , ";
		    out << std::setw(20) << std::setfill(' ') << dasm()     << " , ";
		    out << std::setw( 9) << std::setfill('0') << _ready	    << " , ";
		    out << std::setw( 9) << std::setfill('0') << _issue     << " , ";
		    out << std::setw( 9) << std::setfill('0') << _complete;
		    out << std::endl;
		    out.copyfmt(state);
		}
		virtual bool 		process()				// process this operation
		{
		    _count = counters::operations;
		    counters::operations++;					// increment operation count
		    u64 minissue = ready(); _ready = minissue;			// inputs ready
		    bool issuable = false;					// look for earliest issue possible
		    while (!issuable)
		    {
			issuable = true;
			if (issued.count(minissue) < params::Backend::maxissue) // if there are still issue slots in this cycle
			{
			    for (int i=0; i<throughput(); i++)			// test the next "inverse throughput" cycles
			    {
				if (unit().busy(minissue + i)) 			// if any of them busy, cannot issue
				{
				    issuable = false;
				    break;
				}
			    }
			}
			else
			{
			    issuable = false;
			}
			if (!issuable) minissue++;
		    }
		    _issue = minissue;
		    issued.insert(minissue);					// mark issue on this cycle
		    for (int i=0; i<throughput(); i++)				// mark the unit busy for the next "inverse throughput" cycles
		    {
			unit().claim(minissue + i);
		    }
		    u64 cycle = counters::cycles;				// current cycle count
		    counters::cycles = std::max(cycle, minissue + latency()); 	// current cycle could advance to the end of this operation
		    target(minissue + latency());				// save results and update ready time for output register
		    _complete = minissue + latency();
		    counters::lastissued = minissue;				// update time of last issue
		    if (tracing) output(std::cout);
		    return issue(minissue);
		}
	};

	bool process(operation* op);

	class addi : public operation
	{
	    private:
		gprnum	_RT;
		gprnum 	_RA;
		i16	_SI;
		u32	_idx;
	    public:
		addi(gprnum RT, gprnum RA, i16 SI) { _RT = RT; _RA = RA, _SI = SI; }
		bool execute() 
		{ 
		}
		units::unit& unit() { return units::FXU; }
		void target(u64 cycle) 
		{ 
		    GPR[_RT].busy() = false;
		    do
		    {
			PRF::next %= params::PRF::N; 
			_idx = PRF::next++; 
		    } while (PRF::R[_idx].busy());
		    PRF::R[_idx].busy() = true;
		}
		bool issue(u64 cycle)
		{
		    u32 RES = GPR[_RA].data() + _SI; 
		    GPR[_RT].idx()   = _idx; 
		    GPR[_RT].data()  = RES;
		    GPR[_RT].ready() = cycle + latency(); 
		    return false; 
		}
		u64 ready() { return max(GPR[_RA].ready()); }
		std::string dasm() { std::string str = "addi (p" + std::to_string(_idx) + ", p" + std::to_string(GPR[_RA].idx()) + ", " + std::to_string(_SI) + ")"; return str; }
	};

	class cmpi : public operation
	{
	    private:
		gprnum	_RA;
		i16	_SI;
	    public:
		cmpi(gprnum RA, i16 SI) { _RA = RA; _SI = SI; }
		bool issue(u64 cycle) 
		{
		    flags.LT = false; flags.GT = false; flags.EQ = false;
		    if      (GPR[_RA].data() < _SI) flags.LT = true;
        	    else if (GPR[_RA].data() > _SI) flags.GT = true;
        	    else                            flags.EQ = true;
		    return false; 
		}	
		units::unit& unit() { return units::FXU; }
		void target(u64 cycle) { }
		u64 ready() { return max(GPR[_RA].ready()); }
		std::string dasm() { std::string str = "cmpi (p" + std::to_string(GPR[_RA].idx()) + ", " + std::to_string(_SI) + ")"; return str; }
	};

	class lbz : public operation
	{
	    private:
		gprnum	_RT;
		gprnum	_RA;
		u32	_latency;
		u32	_idx;
	    public:
		lbz(gprnum RT, gprnum RA) { _RT = RT; _RA = RA; _latency = 0; }
		u32 latency() { if(_latency) return _latency; u32 EA = GPR[_RA].data(); _latency = caches::L1.contains(EA,1) ? params::L1::latency : params::MEM::latency; return _latency; }
		units::unit& unit() { return units::LDU; }
		void target(u64 cycle) 
		{ 
		    GPR[_RT].busy() = false;
		    do
		    {
			PRF::next %= params::PRF::N; 
			_idx = PRF::next++; 
		    } while (PRF::R[_idx].busy());
		    PRF::R[_idx].busy() = true;
		}
		bool issue(u64 cycle)
		{
		    u32 EA = GPR[_RA].data(); 			// compute effective address of load
		    u8* data = caches::L1.fill(EA, 1);		// fill the cache with the line, if not already there
		    u32 RES = *((u8*)data);			// get data from the cache
		    GPR[_RT].idx()   = _idx;
		    GPR[_RT].data()  = RES;
		    GPR[_RT].ready() = cycle + latency(); 
		    return false; 
		}
		u64 ready() { return max(GPR[_RA].ready()); }
		std::string dasm() { std::string str = "lbz (p" + std::to_string(_idx) + ", p" + std::to_string(GPR[_RA].idx()) + ")"; return str; }
	};

	class stb : public operation
	{
	    private:
		gprnum	_RS;
		gprnum	_RA;
		u32	_latency;
	    public:
		stb(gprnum RS, gprnum RA) { _RS = RS; _RA = RA; _latency = 0; }
		bool issue(u64 cycle) 
		{
		    uint32_t EA = GPR[_RA].data();		// compute effective address of store
		    u8* data = caches::L1.fill(EA, 1);		// fill the cache with the line, if not already there
		    *((u8*)data) = GPR[_RS].data() & 0xff;	// write data to cache
                    MEM[EA] = GPR[_RS].data() & 0xff;		// write to memory as well, since L1 is write-through!
		    return false; 
		}
		u32 latency() { if(_latency) return _latency; u32 EA = GPR[_RA].data(); _latency = caches::L1.contains(EA,1) ? params::L1::latency : params::MEM::latency; return _latency; }
		units::unit& unit() { return units::STU; }
		void target(u64 cycle) { }
		u64 ready() { return max(GPR[_RA].ready(), GPR[_RS].ready()); }
		std::string dasm() { std::string str = "stb (p" + std::to_string(GPR[_RS].idx()) + ", p" + std::to_string(GPR[_RA].idx()) + ")"; return str; }
	};

	class lfd : public operation
	{
	    private:
		fprnum	_FT;
		gprnum	_RA;
		u32	_latency;
		u32	_idx;
	    public:
		lfd(fprnum FT, gprnum RA) { _FT = FT; _RA = RA; _latency = 0; }
		u32 latency() { if(_latency) return _latency; u32 EA = GPR[_RA].data(); _latency = caches::L1.contains(EA,8) ? params::L1::latency : params::MEM::latency; return _latency; }
		units::unit& unit() { return units::LDU; }
		void target(u64 cycle) 
		{ 
		    FPR[_FT].busy() = false;
		    do
		    {
			PRF::next %= params::PRF::N; 
			_idx = PRF::next++; 
		    } while (PRF::R[_idx].busy());
		    PRF::R[_idx].busy() = true;
		}
		bool issue(u64 cycle)
		{
		    u32 EA = GPR[_RA].data();			// compute effective address of load
		    u8* data = caches::L1.fill(EA, 8);		// fill the cache with the line, if not already there
		    double RES = *((double*)data);		// get data from the cache
		    FPR[_FT].idx()   = _idx;
		    FPR[_FT].data()  = RES;
		    FPR[_FT].ready() = cycle + latency(); 
		    return false;
		}
		u64 ready() { return max(GPR[_RA].ready()); }
		std::string dasm() { std::string str = "lfd (p" + std::to_string(_idx) + ", p" + std::to_string(GPR[_RA].idx()) + ")"; return str; }
	};

	class stfd : public operation
	{
	    private:
		fprnum _FS;
		gprnum _RA;
		u32	_latency;
	    public:
		stfd(fprnum FS, gprnum RA) { _FS = FS; _RA = RA; _latency = 0; }
		bool issue(u64 cycle)
		{
		    u32 EA = GPR[_RA].data();				// compute effective address of store
		    u8* data = caches::L1.fill(EA, 8);			// fill the cache with the line, if not already there
		    *((double*)(MEM.data() + EA)) = FPR[_FS].data();	// write data to cache
		    *((double*)data) = FPR[_FS].data();			// write to memory as well, since L1 is write-through!
		    return false;
		}
		u32 latency() { if(_latency) return _latency; u32 EA = GPR[_RA].data(); _latency = caches::L1.contains(EA,8) ? params::L1::latency : params::MEM::latency; return _latency; }
		units::unit& unit() { return units::STU; }
		void target(u64 cycle) { }
		u64 ready() { return max(GPR[_RA].ready(), FPR[_FS].ready()); }
		std::string dasm() { std::string str = "stfd (p" + std::to_string(FPR[_FS].idx()) + ", p" + std::to_string(GPR[_RA].idx()) + ")"; return str; }
	};

	class b : public operation
	{
	    private:
		i16	_BD;
	    public:
		b(i16 BD) { _BD = BD; }
		bool issue(u64 cycle) { NIA = CIA + _BD; return true; }
		units::unit& unit() { return units::BRU; }
		void target(u64 cycle) { }
		u64 ready() { return 0; }
		std::string dasm() { std::string str = "b (" + std::to_string(_BD) + ")"; return str; }
	};

	class beq : public operation
	{
	    private:
		i16	_BD;
	    public:
		beq(i16 BD) { _BD = BD; }
		bool issue(u64 cycle) { if (flags.EQ) { NIA = CIA + _BD; return true; } else return false; }
		units::unit& unit() { return units::BRU; }
		void target(u64 cycle) { }
		u64 ready() { return 0; }
		std::string dasm() { std::string str = "beq (" + std::to_string(_BD) + ")"; return str; }
	};

	class zd : public operation
	{
	    private:
		fprnum	_FT;
		u32	_idx;
	    public:
		zd(fprnum FT) { _FT = FT; }
		units::unit& unit() { return units::FPU; }
		void target(u64 cycle) 
		{ 
		    FPR[_FT].busy() = false;
		    do
		    {
			PRF::next %= params::PRF::N; 
			_idx = PRF::next++; 
		    } while (PRF::R[_idx].busy());
		    PRF::R[_idx].busy() = true;
		}
		bool issue(u64 cycle)
		{
		    FPR[_FT].idx()   = _idx;
		    FPR[_FT].data()  = 0.0;
		    FPR[_FT].ready() = cycle + latency(); 
		    return false;
		}
		u64 ready() { return 0; }
		std::string dasm() { std::string str = "zd (p" + std::to_string(_idx) + ")"; return str; }
	};

	class fmul : public operation
	{
	    private:
		fprnum 	_FT;
		fprnum	_FA;
		fprnum	_FB;
		u32	_idx;
	    public:
		fmul(fprnum FT, fprnum FA, fprnum FB) { _FT = FT; _FA = FA; _FB = FB; }
		units::unit& unit() { return units::FPU; }
		void target(u64 cycle) 
		{ 
		    FPR[_FT].busy() = false;
		    do
		    {
			PRF::next %= params::PRF::N; 
			_idx = PRF::next++; 
		    } while (PRF::R[_idx].busy());
		    PRF::R[_idx].busy() = true;
		}
		bool issue(u64 cycle)
		{
		    double RES = FPR[_FA].data() * FPR[_FB].data(); 
		    FPR[_FT].idx()   = _idx;
		    FPR[_FT].data()  = RES;
		    FPR[_FT].ready() = cycle + latency(); 
		    return false; 
		}
		u64 ready() { return max(FPR[_FA].ready(), FPR[_FB].ready()); }
		std::string dasm() { std::string str = "fmul (p" + std::to_string(_idx) + ", p" + std::to_string(FPR[_FA].idx()) + ", p" + std::to_string(FPR[_FB].idx()) + ")"; return str; }
	};

	class fadd : public operation
	{
	    private:
		fprnum 	_FT;
		fprnum	_FA;
		fprnum	_FB;
		u32	_idx;
	    public:
		fadd(fprnum FT, fprnum FA, fprnum FB) { _FT = FT; _FA = FA; _FB = FB; }
		bool execute() 
		{ 
		}
		units::unit& unit() { return units::FPU; }
		void target(u64 cycle) 
		{ 
		    FPR[_FT].busy() = false;
		    do
		    {
			PRF::next %= params::PRF::N; 
			_idx = PRF::next++; 
		    } while (PRF::R[_idx].busy());
		    PRF::R[_idx].busy() = true;
		}
		bool issue(u64 cycle)
		{
		    double RES = FPR[_FA].data() + FPR[_FB].data(); 
		    FPR[_FT].idx()   = _idx;
		    FPR[_FT].data()  = RES;
		    FPR[_FT].ready() = cycle + latency(); 
		    return false; 
		}
		u64 ready() { return max(FPR[_FA].ready(), FPR[_FB].ready()); }
		std::string dasm() { std::string str = "fadd (p" + std::to_string(_idx) + ", p" + std::to_string(FPR[_FA].idx()) + ", p" + std::to_string(FPR[_FB].idx()) + ")"; return str; }
	};
    };

    namespace instructions
    {
	class instruction
	{
	    private:
		static bool	first;	// first instruction processed

		u64		_count;	// instruction #

	    public:
		static void		zero() { first = true; }
		virtual bool 		process() = 0;
		virtual std::string	dasm() = 0;
		u64&	count()		{ return _count; }
		const u64& count() const{ return _count; }
		void output(std::ostream& out)
		{
		    if (first)
		    {
			out << "  instr # ,          instruction ,      op # ,            operation ,     ready ,    issued ,  complete" << std::endl;
			first = false;
		    }

		    std::ios state(nullptr);
		    state.copyfmt(out);
		    out << std::setw( 9) << std::setfill('0') << _count     << " , ";
		    out << std::setw(20) << std::setfill(' ') << dasm()     << " , ";
		    out.copyfmt(state);
		}
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
		std::string dasm() { std::string str = "addi (r" + std::to_string(_RT) + ", r" + std::to_string(_RA) + ", " + std::to_string(_SI) + ")"; return str; }
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
		std::string dasm() { std::string str = "cmpi (r" + std::to_string(_RA) + ", " + std::to_string(_SI) + ")"; return str; }
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
		std::string dasm() { std::string str = "lbz (r" + std::to_string(_RT) + ", r" + std::to_string(_RA) + ")"; return str; }
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
		std::string dasm() { std::string str = "stb (r" + std::to_string(_RS) + ", r" + std::to_string(_RA) + ")"; return str; }
	};

	class beq : public instruction
	{
	    private:
		i16	_BD;
	    public:
		beq(i16 BD) { _BD = BD; }
		bool process() { return operations::process(new operations::beq(_BD)); }
		static bool execute(i16 BD) { return instructions::process(new beq(BD)); }
		std::string dasm() { std::string str = "beq (" + std::to_string(_BD) + ")"; return str; }
	};

	class b : public instruction
	{
	    private:
		i16	_BD;
	    public:
		b(i16 BD) { _BD = BD; }
		bool process() { return operations::process(new operations::b(_BD)); }
		static bool execute(i16 BD) { return instructions::process(new b(BD)); }
		std::string dasm() { std::string str = "b (" + std::to_string(_BD) + ")"; return str; }
	};

	class zd : public instruction
	{
	    private:
		fprnum	_FT;
	    public:
		zd(fprnum FT) { _FT = FT; }
		bool process() { return operations::process(new operations::zd(_FT)); }
		static bool execute(fprnum FT) { return instructions::process(new zd(FT)); }
		std::string dasm() { std::string str = "zd (f" + std::to_string(_FT) + ")"; return str; }
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
		std::string dasm() { std::string str = "fmul (f" + std::to_string(_FT) + ", f" + std::to_string(_FA) + ", f" + std::to_string(_FB) + ")"; return str; }
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
		std::string dasm() { std::string str = "fadd (f" + std::to_string(_FT) + ", f" + std::to_string(_FA) + ", f" + std::to_string(_FB) + ")"; return str; }
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
		std::string dasm() { std::string str = "lfd (f" + std::to_string(_FT) + ", r" + std::to_string(_RA) + ")"; return str; }
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
		std::string dasm() { std::string str = "stfd (f" + std::to_string(_FS) + ", r" + std::to_string(_RA) + ")"; return str; }
	};
    };
};

#endif
