#include<simple.hh>
#include<mxv.hh>

namespace simple
{
    void mxv
    (
        double		*y,	// GPR[3]
	double		*a,	// GPR[4]
	double		*x,	// GPR[5]
	uint32_t	 m,	// GPR[6]
	uint32_t	 n	// GPR[7]
    )
    {

loopi:
	cmpi(r6, 0);		// while (m != 0)
	beq(end);
	addi(r8, r5, 0);	// GPR[8] = x
	addi(r9, r7, 0);	// GPR[9] = n
	zd(f0);			// f0 = 0.0

loopj:
	cmpi(r9,0);		// while (n != 0)
	beq(nexti);
	lfd(f1, r8);		// f1 = x[j]
	lfd(f2, r4);		// f2 = a[i,j]
	fmul(f3, f2, f1);	
	fadd(f0, f0, f3);	// f0 = f0 + a[i,j]*x[j]
	addi(r8, r8, 8);	// x++
	addi(r4, r4, 8);	// a++
	addi(r9, r9, -1);	// n--
	b(loopj);

nexti:
	stfd(f0, r3);		// y[i] = f0
	addi(r3, r3, 8);	// y++
	addi(r6, r6, -1);	// m--
	b(loopi);
end:    
	return;
    }
};
