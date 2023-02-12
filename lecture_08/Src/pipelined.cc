#include<pipelined.hh>

namespace pipelined
{
    bool	tracing = true;
    bool	operations::operation::first = true;
    bool	instructions::instruction::first = true;

    const u32	params::MEM::N = 1024*1024;			// 1 MiB of main memory
    const u32 	params::MEM::latency = 300;

    const u32 	params::L1::latency = 2;
    const u32	params::L1::nsets = 16;
    const u32 	params::L1::nways = 4;
    const u32	params::L1::linesize = 8;

    const u32 	params::L2::latency = 4;
    const u32	params::L2::nsets = 64;
    const u32 	params::L2::nways = 4;
    const u32	params::L2::linesize = 8;

    const u32 	params::L3::latency = 8;
    const u32	params::L3::nsets = 64;
    const u32 	params::L3::nways = 16;
    const u32	params::L3::linesize = 8;

    const u32	params::GPR::N = 16;
    const u32 	params::FPR::N = 8;
    const u32	params::PRF::N = 64;

    const u32	params::Backend::maxissue = 1;
    const u32	params::Frontend::DECODE::latency = 1;
    const u32	params::Frontend::DISPATCH::latency = 1;

    std::vector<u8>		MEM(params::MEM::N);
    std::vector<reg<u32>>	GPR(params::GPR::N);
    std::vector<reg<double>>	FPR(params::FPR::N);
    std::vector<preg<u64>>	PRF::R(params::PRF::N);
    u32 			PRF::next = 0;

    units::unit			units::LDU;
    units::unit			units::STU;
    units::unit			units::FXU;
    units::unit			units::FPU;
    units::unit			units::BRU;

    std::multiset<u64>		operations::issued;

    namespace PRF
    {
	u32	find_next()
	{
	    return find_earliest();
	}

	u32	find_earliest()
	{
	    u32 idx;
	    do						// find a first candidate
	    {
		PRF::next %= params::PRF::N;
		idx = PRF::next++;
	    } while (PRF::R[idx].busy());
	    for (u32 i=0; i<params::PRF::N; i++)	// look for a possibly better candidate
		if ((!(PRF::R[i].busy())) && (PRF::R[i].used() < PRF::R[idx].used())) idx = i;
	    PRF::R[idx].busy() = true;
	    return idx;
	}

	u32	find_first()
	{
	    u32 idx;
	    do
	    {
		PRF::next %= params::PRF::N;
		idx = PRF::next++;
	    } while (PRF::R[idx].busy());
	    PRF::R[idx].busy() = true;
	    return idx;
	}
    };

    namespace caches
    {
        cache   L1D(params::L1::nsets, params::L1::nways, params::L1::linesize);
	cache	L1I(params::L1::nsets, params::L1::nways, params::L1::linesize);
	cache	L2 (params::L2::nsets, params::L2::nways, params::L2::linesize);
	cache	L3 (params::L3::nsets, params::L3::nways, params::L3::linesize);

        cache::cache(uint32_t nsets, uint32_t nways, uint32_t linesize) : _sets(nsets)
        {
            _nsets = nsets;
            _nways = nways;
            _linesize = linesize;

	    accesses = 0;
	    hits = 0;
	    misses = 0;

            entry empty(linesize); empty.valid = false; empty.touched = 0;
            set   init(nways); for (uint32_t i=0; i<nways; i++) init[i] = empty;
            for (uint32_t i=0; i<nsets; i++) _sets[i] = init;
        }

        void cache::clear()
        {
	    accesses = 0;
	    hits = 0;
	    misses = 0;

            for (uint32_t setix=0; setix<nsets(); setix++)
                for (uint32_t wayix=0; wayix<nways(); wayix++)
                {
                    sets()[setix][wayix].valid = false;
                    sets()[setix][wayix].touched = 0;
                    sets()[setix][wayix].addr = 0;
                }
        }

        uint32_t cache::linesize() const
        {
            return _linesize;
        }

        uint32_t cache::nsets() const
        {
            return _nsets;
        }

        uint32_t cache::nways() const
        {
            return _nways;
        }

        uint32_t cache::capacity() const
        {
            return _nsets * _nways * _linesize;
        }

        array &cache::sets()
        {
            return _sets;
        }

	bool 	cache::contains(u32 EA, u32 L, u32 &setix, u32 &wayix)
	{
	    u32	lineaddr = EA / linesize();			// compute line address of first byte
	    assert(lineaddr == ((EA+L-1) / linesize())); 	// assert that last byte is in the same line
            setix = lineaddr % nsets();				// compute set index from line address
            for (wayix = 0; wayix < nways(); wayix++)		// look for address in one of the ways of the set
            {
                if (sets()[setix][wayix].valid && (sets()[setix][wayix].addr == lineaddr)) break;
            }
            return (wayix < nways());
	}

	bool 	cache::contains(u32 EA, u32 L)
	{
	    u32 setix; u32 wayix;
	    return contains(EA, L, setix, wayix);
	}

	u32	cache::lineaddr(u32 EA)
	{
	    return EA - offset(EA);
	}

	u32	cache::offset(u32 EA)
	{
	    return EA % linesize();
	}

	entry*	cache::find(u32 EA, u32 L)
	{
	    u32 setix; u32 wayix; 
	    if (contains(EA, L, setix, wayix)) 	return &(sets()[setix][wayix]);
	    else 				return 0;
	}

	/*
	u8*	cache::fill(u32 EA, u32 L)
	{
	    u32 setix; u32 wayix; u32 offset = EA % linesize(); u32 lineaddr = EA / linesize();
	    if (contains(EA, L, setix, wayix))
	    {
		// This is a hit! just return the contents at the offset
		sets()[setix][wayix].touched = counters::cycles;
		return sets()[setix][wayix].data.data() + offset;
	    }
	    else
	    {
	    	// This is a miss! We need to allocate an entry and bring data from memory
                // find the LRU entry
                u64 lasttouch = counters::cycles;
                u32 lru = nways();
                for (wayix = 0; wayix < nways(); wayix++)
                {
                    if (!sets()[setix][wayix].valid)
                    {
                        // invalid entry, can use this one as the lru
                        lru = wayix;
                        break;
                    }
                    if (sets()[setix][wayix].touched <= lasttouch)
                    {
                        // older than current candidate - update
                        lru = wayix;
                        lasttouch = sets()[setix][wayix].touched;
                    }
                }
                assert(lru < nways());
                sets()[setix][lru].valid = true;					// entry is now valid
                sets()[setix][lru].addr = lineaddr;					// it has this line address
                sets()[setix][lru].touched = counters::cycles;				// it was just touched
		for (u32 i=0; i<linesize(); i++) 
		    sets()[setix][lru].data[i] = MEM[lineaddr * linesize() + i];	// fill the entry with L bytes from memory, starting at addrress EA
		return sets()[setix][lru].data.data() + offset;				// return the contents
	    }
	}
	*/

	u8*	cache::fill(u32 EA, u32 L, std::vector<u8> &M)
	{
	    u32 setix; u32 wayix; u32 offset = EA % linesize(); u32 lineaddr = EA / linesize();
	    if (contains(EA, L, setix, wayix))
	    {
		// This is a hit! just return the contents at the offset
		return sets()[setix][wayix].data.data() + offset;
	    }
	    else
	    {
	    	// This is a miss! We need to allocate an entry and bring data from memory
                // find the LRU entry
                u64 lasttouch = counters::cycles;
                u32 lru = nways();
                for (wayix = 0; wayix < nways(); wayix++)
                {
                    if (!sets()[setix][wayix].valid)
                    {
                        // invalid entry, can use this one as the lru
                        lru = wayix;
                        break;
                    }
                    if (sets()[setix][wayix].touched <= lasttouch)
                    {
                        // older than current candidate - update
                        lru = wayix;
                        lasttouch = sets()[setix][wayix].touched;
                    }
                }
                assert(lru < nways());
                sets()[setix][lru].valid = true;					// entry is now valid
                sets()[setix][lru].addr = lineaddr;					// it has this line address
                sets()[setix][lru].touched = counters::cycles;				// it was just touched
		for (u32 i=0; i<linesize(); i++) 
		    sets()[setix][lru].data[i] = M[lineaddr * linesize() + i];		// fill the entry with L bytes from memory, starting at addrress EA
		return sets()[setix][lru].data.data() + offset;				// return the contents
	    }
	}

	void 	cache::hit(u32 EA, u32 L)
	{
	    hits++;
	    u32 setix; u32 wayix;
	    if (contains(EA, L, setix, wayix))
	    {
		sets()[setix][wayix].touched = counters::cycles;
	    }
	}

	void	cache::miss(u32 EA, u32 L)
	{
	    misses++;
	}

	void 	cache::access(u32 EA, u32 L)
	{
	    accesses++;
	}
    };

    u8*	pipelined::operations::load
    (
	u32	EA,
	u32 	L
    )
    {
	caches::L1D.access(EA, L);
	if (caches::L1D.contains(EA, L))
	{
	    // this is an L1 hit
	    caches::L1D.hit(EA, L);
	}
	else
	{
	    // this is an L1 miss
	    caches::L1D.miss(EA, L);

	    // load cache line from memory
	    caches::L1D.fill(EA, L, MEM);
	}
	return caches::L1D.find(EA, L)->data.data() + caches::L1D.offset(EA);
    }

    flags_t     flags;                          // flags

    uint32_t    CIA;                            // current instruction address
    uint32_t    NIA;                            // next instruction address

    uint64_t    counters::instructions = 0;     // instruction counter
    uint64_t    counters::operations = 0;       // operation counter
    uint64_t    counters::cycles = 0;           // cycle counter
    uint64_t    counters::lastissued = 0;       // last issue cycle
    uint64_t    counters::lastfetched = 0;      // last fetch cycle

    void zeromem()
    {
        for (uint32_t i=0; i<params::MEM::N; i++) MEM[i] = 0;
    }

    void zeroctrs()
    {
    	operations::operation::zero();;
    	instructions::instruction::zero();
	counters::instructions = 0;
	counters::operations = 0;
	counters::cycles = 0;
	counters::lastissued = 0;
	counters::lastfetched = 0;
	PRF::next = 0;
	for (u32 i=0; i<params::GPR::N; i++) GPR[i].idx() = PRF::next++;
	for (u32 i=0; i<params::FPR::N; i++) FPR[i].idx() = PRF::next++;
	for (u32 i=0; i<params::PRF::N; i++) PRF::R[i].ready() = 0;
	for (u32 i=0; i<params::PRF::N; i++) PRF::R[i].busy() = false;
	for (u32 i=0; i<params::PRF::N; i++) PRF::R[i].used() = 0;
	for (u32 i=0; i<params::GPR::N; i++) GPR[i].ready() = 0;
	for (u32 i=0; i<params::GPR::N; i++) GPR[i].busy() = true;
	for (u32 i=0; i<params::FPR::N; i++) FPR[i].ready() = 0;
	for (u32 i=0; i<params::FPR::N; i++) FPR[i].busy() = true;
	units::FXU.clear();
	units::FPU.clear();
	units::LDU.clear();
	units::STU.clear();
	units::BRU.clear();
	operations::issued.clear();
	pipelined::caches::L1D.clear();
	pipelined::caches::L1I.clear();
	pipelined::caches:: L2.clear();
	pipelined::caches:: L3.clear();
    }

    namespace operations
    {
	bool process(operation* op, u64 dispatch) { return op->process(dispatch); } 
    };

    namespace instructions
    {
	bool process(instruction* inst) 
	{ 
	    inst->count() = counters::instructions;
	    counters::instructions++;
	    inst->fetch();	// fetch time
	    inst->decode();	// decode time
	    inst->dispatch();	// dispatch time
	    if (tracing) inst->output(std::cout);
	    return inst->process(); 
	}
    };
};
