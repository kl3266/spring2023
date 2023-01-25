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
	uint32_t N = simple::GPR[7];

	for (uint32_t i=0; i<M ; i++)
	{
	    addi(r8, r5, 0);
	    zd(f0);
	    for (uint32_t j=0; j<N; j++)
	    {
		lfd(f1, r8);
		lfd(f2, r4);
		simple::FPR[0] += simple::FPR[2] * simple::FPR[1];
		addi(r8, r8, 8);
		addi(r4, r4, 8);
	    }
	    stfd(f0, r3);
	    addi(r3, r3, 8);
	}
end:    return;
    }
};
