#include<pipelined.hh>
#include<mxv.hh>
#include<stdio.h>

using namespace pipelined;

int main
(
    int		  argc,
    char	**argv
)
{
    printf("L1: %u bytes of capacity, %u sets, %u-way set associative, %u-byte line size\n", 
	   pipelined::caches::L1.capacity(), pipelined::caches::L1.nsets(), pipelined::caches::L1.nways(), pipelined::caches::L1.linesize());

    for (uint32_t m = 2; m <= 32; m *= 2) for (uint32_t n = m/2; n <= m; n *= 2)
    {
	pipelined::zeromem();

	const uint32_t M = m;
	const uint32_t N = n;

	const uint32_t Y = 0;
	const uint32_t X = Y + M*sizeof(double);
	const uint32_t A = X + N*sizeof(double);

	for (uint32_t i=0; i<M; i++) *((double*)(pipelined::MEM.data() + Y + i*sizeof(double))) = 0.0;
	for (uint32_t j=0; j<N; j++) *((double*)(pipelined::MEM.data() + X + j*sizeof(double))) = (double)j;
	for (uint32_t i=0; i<M; i++) for (uint32_t j=0; j<N; j++) *((double*)(pipelined::MEM.data() + A + (i*N+j)*sizeof(double))) = (double)i;

	pipelined::GPR[3].data() = Y;
	pipelined::GPR[4].data() = A;
	pipelined::GPR[5].data() = X;
	pipelined::GPR[6].data() = M;
	pipelined::GPR[7].data() = N;
	
	pipelined::zeroctrs();
	pipelined::caches::L1.clear();
	pipelined::mxv(0,0,0,0,0);
	
	if (pipelined::tracing) printf("\n");
	printf("M = %6d, N = %6d : instructions = %6lu, cycles = %8lu, L1 accesses= %6lu, L1 misses = %6lu :",
		M, N, pipelined::counters::operations, pipelined::counters::cycles, pipelined::caches::L1.accesses, pipelined::caches::L1.misses);
	bool pass = true;
	for (uint32_t i=0; i<M; i++)
	{
	    double y = *((double*)(pipelined::MEM.data() + Y + i*sizeof(double)));
	    // printf("\ny[%2d] = %10.0f", i, y);
	    if (y != ((N*(N-1))/2)*i) { pass = false; }
	}
	// printf("\n");
	if (pass) printf("PASS\n");
	else      printf("FAIL\n");
    }
    
    return 0;
}
