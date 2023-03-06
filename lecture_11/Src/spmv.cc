#include<pipelined.hh>
#include<ISA.hh>
#include<spmv.hh>

namespace pipelined
{
    void spmv
    (
        double *y,               // GPR[3]
        uint32_t nnz,           // GPR[4]
        uint32_t *i,            // GPR[5]
        uint32_t *j,            // GPR[6]
        double *a,               // GPR[7]
        double *x                // GPR[8]
    )
    {
loop:   cmpi(r4, 0);            // k == 0?
        beq(end);               // while(k != 0)
        lw(r11,r5);             // load u32 i[k] value into GPR[11]
        lw(r10,r6);             // load u32 j[k] value into GPR[10]
        muli(r10, r10, 8);       // GPR[10] = j[k] * 8
        muli(r11, r11, 8);       // GPR[11] = i[k] * 8
        add(r10, r8, r10);     // &x + 8 * j[k]
        add(r11, r3, r11);     // &y + 8 * i[k]
        lfd(f0, r10);           // load x[j[k]] into f0
        lfd(f1, r11);           // load y[i[k]] into f1
        lfd(f2, r7);            // load a[k] into f2
        fmul(f2, f2, f0);       // a[k] * x[j[k]]
        fadd(f1, f1, f2);       // y[i[k]] += a[k] * x[j[k]] (accumulate)
        stfd(f1, r11);           // store the result into y
        addi(r5, r5, 4);        // i+=4 bc u32
        addi(r6, r6, 4);        // j+=4 bc u32
        addi(r7, r7, 8);        // a+=8 bc double
        addi(r4, r4, -1);       // k--
        b(loop);                // end while
end:    return;
    }
};
