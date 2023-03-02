#include<pipelined.hh>
#include<ISA.hh>
#include<vmemcpy.hh>

namespace pipelined
{
    void *vmemcpy               // GPR[3]
    (
        void            *dest,  // GPR[3]
        const void      *src,   // GPR[4]
        size_t           n, u32 v_size    // GPR[5] v_size  // GPR[8]
    )
    {
        addi(r7, r3, 0);        // preserve GPR[3] so that we can just return it
loop:	cmp(r5, r8);		// n == v_size?
	blt(remain);		// while(n >= v_size)
	vlb(v0, r4);		// load vector of bytes from *src
	vstb(v0, r7);		// store vector of bytes to *dest
	add(r4, r4, r8);	// src += v_size
	add(r7, r7, r8);	// dest += v_size
	sub(r5, r5, r8);	// n -= v_size
	b(loop);		// end while
remain: cmpi(r5, 0);            // n == 0?
        beq(end);               // while(n != 0)
        lbz(r6, r4);            // load byte from *src
        stb(r6, r7);            // store byte to *dest
        addi(r4, r4, 1);        // src++
        addi(r7, r7, 1);        // dest++
        addi(r5, r5, -1);       // n--
        b(remain);              // end while
end:    return dest;
    }
};
