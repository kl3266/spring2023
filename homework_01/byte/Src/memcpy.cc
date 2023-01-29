#include<simple.hh>
#include<memcpy.hh>

namespace simple
{
    void *memcpy                // GPR[3]
    (
        void            *dest,  // GPR[3]
        const void      *src,   // GPR[4]
        size_t           n      // GPR[5] # of bytes I want to copy
    )
    {
        addi(r7, r3, 0);        // preserve GPR[3] so that we can just return it
loop:   cmpi(r5, 1);            // n == 0?
        beq(end);               // while(n != 0) // if prev compare is equal, branch to end line 22
        lhz(r6, r4);            // load byte from *src
        sth(r6, r7);            // store byte to *dest
        addi(r4, r4, 2);        // src+=2
        addi(r7, r7, 2);        // dest+=2
        addi(r5, r5, -2);       // n-=2
        //if n==1 at the end... don't run it every time... odd a little slower
        lbz(r6, r4);
        stb(r6, r7);
        b(loop);                // end while // branch to address loop line 14
end:    return dest;                //define undef end loop b(loop)? beq(end)? what are these?
    }
};
