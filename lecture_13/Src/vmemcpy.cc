#include<pipelined.hh>
#include<ISA.hh>
#include<vmemcpy.hh>

namespace pipelined
{
    void *vmemcpy               // GPR[3]
    (
        void            *dst,   // GPR[3]
        const void      *src,   // GPR[4]
        size_t           n      // GPR[5]
    )
    {
        addi(r7, r3, 0);        // preserve GPR[3] so that we can just return it
loop:	cmpi(r5,0);		// n == 0?
	beq(end);		// while(n != 0)
	vmaskb(v0,r5);		// VM = vmaskb(n)
	vlb(v1, r4, v0);	// load  vector of bytes from *src, guarded by VM
	vstb(v1, r7, v0);	// store vector of bytes to   *dst, guarded by VM
	vpopcnt(r8, v0);	// CNT = # of entries in VM
	add(r4, r4, r8);	// src += CNT
	add(r7, r7, r8);	// dst += CNT
	sub(r5, r5, r8);	// n   -= CNT
	b(loop);		// end while
end:    return dst;
    }
};
