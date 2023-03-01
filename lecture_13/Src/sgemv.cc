#include<pipelined.hh>
#include<ISA.hh>
#include<sgemv.hh>

namespace pipelined
{
    void sgemv
    (
        float		*y,	// GPR[3]
	float		*A,	// GPR[4]
	float		*x,	// GPR[5]
	uint32_t	 m,	// GPR[6]
	uint32_t	 n,	// GPR[7]
	uint32_t	 ldA	// GPR[8]
    )
    {
	muli(r8, r8, 4);		// r8 = ldA in bytes
loopj:  cmpi(r7, 0);			// n == 0?
	beq(end); 			// while (n != 0)
	vmaskw(v0, r6);			// VM = vmaskw(m)
	vlspltsp(v1, r5, v0);		// v1<VM> = x[j]
	addi(r9, r6, 0);		// r9 = m
	addi(r10, r4, 0);		// r10 = A[:,j]
	addi(r13, r3, 0);		// r13 = y
loopi:  cmpi(r9,0);			// m == 0?
	beq(nextj); 			// while (m != 0)
	vmaskw(v0, r9);			// VM = vmaskw(m)
	vlfs(v2, r10, v0);		// v2<VM> = A[i+0:i+VL,j]
	vlfs(v3, r13, v0);		// v3<VM> = y[i+0:i+VL]
	vfmulsp(v2, v2, v1, v0);	// v2<VM> = A[i+0:i+VL,j]*x[j]
	vfaddsp(v3, v3, v2, v0);	// v3<VM> = y[i+0:i+VL] + A[i+0:i+VL,j]*x[j]
	vstfs(v3, r13, v0);		// y[i+0:i+VL]<VM> = v3
	vpopcnt(r11, v0);		// CNT = # of entries in VM
	muli(r12, r11, 4);		// r12 = 4*CNT
	add(r10, r10, r12);		// A[:,j] += 4*CNT
	add(r13, r13, r12);		// y      += 4*CNT
	sub(r9, r9, r11);		// m      -= CNT
	b(loopi);			// i++	
nextj:  add(r4, r4, r8);		// r4 = A[:,j+1]
	addi(r5, r5, 4);		// x++
	addi(r7, r7, -1);		// n--
	b(loopj);			// j++
end:    return;
    }
};
