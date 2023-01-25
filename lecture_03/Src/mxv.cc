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
	uint32_t M = simple::GPR[6];

	while(simple::GPR[6])
	{
	    addi(r8, r5, 0);
	    addi(r9, r7, 0);
	    zd(f0);

	    while(simple::GPR[9])
	    {
		lfd(f1, r8);
		lfd(f2, r4);
		simple::FPR[0] += simple::FPR[2] * simple::FPR[1];
		addi(r8, r8, 8);
		addi(r4, r4, 8);
		simple::GPR[9]--;
	    }
	    stfd(f0, r3);
	    addi(r3, r3, 8);
	    simple::GPR[6]--;
	}
end:    return;
    }
};
