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
	uint32_t Y = simple::GPR[3];
	uint32_t A = simple::GPR[4];
	uint32_t X = simple::GPR[5];
	uint32_t M = simple::GPR[6];
	uint32_t N = simple::GPR[7];

	for (uint32_t i=0; i<M ; i++)
	{
	    zd(f0);
	    for (uint32_t j=0; j<N; j++)
	    {
		simple::FPR[0] += *((double*)(simple::MEM + A + (i*N+j)*sizeof(double))) * *((double*)(simple::MEM + X + j*sizeof(double)));
	    }
	    stfd(f0, r3);
	    addi(r3, r3, 8);
	}
end:    return;
    }
};
