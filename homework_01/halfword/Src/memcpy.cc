#include<simple.hh>
#include<memcpy.hh>

namespace simple
{
    void *memcpy                // GPR[3] - why is this GPR[3]?
    (
        void            *dest,  // GPR[3]
        const void      *src,   // GPR[4]
        size_t           n      // GPR[5] - why is this GPR[5]?
    )
    { // where do lhz and sth go?
        addi(r7, r3, 0);        // preserve GPR[3] so that we can just return it - is r7 = GPR[7] and r3 = GPR[3]?
loop:   cmpi(r5, 0);            // n == 0?
        beq(end);               // while(n != 0) - what is end?
        lbz(r6, r4);            // load byte from *src - why is *src and *dest r4 and r7?
        stb(r6, r7);            // store byte to *dest
        addi(r4, r4, 1);        // src++ - why increment pointers?
        addi(r7, r7, 1);        // dest++
        addi(r5, r5, -1);       // n-- - why decrement n?
        b(loop);                // end while
end:    return dest;
    }
};
