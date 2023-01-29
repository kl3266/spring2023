#include<pipelined.hh>
#include<memcpy.hh>
#include<stdio.h>

int main
(
    int		  argc,
    char	**argv
)
{
    printf("L1: %u bytes of capacity, %u sets, %u-way set associative, %u-byte line size\n", 
	   pipelined::caches::L1.capacity(), pipelined::caches::L1.nsets(), pipelined::caches::L1.nways(), pipelined::caches::L1.linesize());
    pipelined::zeromem();
    const uint32_t N = 1024;
    for (uint32_t i=0; i<N; i++) pipelined::MEM[i] = rand() % 0xff;
    for (uint32_t n = 1; n<=N; n *= 2)
    {
	pipelined::GPR[3].data() = 1024;
	pipelined::GPR[4].data() = 0;
	pipelined::GPR[5].data() = n;
	
	pipelined::zeroctrs();
	pipelined::caches::L1.clear();
	pipelined::memcpy(0,0,0);

	double rate = (double)pipelined::counters::cycles/(double)n;
	
	printf("n = %6d : instructions = %6lu, cycles = %6lu, L1 accesses= %6lu, L1 hits = %6lu",
		n, pipelined::counters::operations, pipelined::counters::cycles, pipelined::counters::L1::accesses, pipelined::counters::L1::hits);
	printf(", cyc/B = %10.2f", rate);

	bool pass = true;
	for (uint32_t i=0; i<n; i++) if (pipelined::MEM[i] != pipelined::MEM[N+i]) pass = false;
	if (pass) printf(" | PASS\n");
	else      printf(" | FAIL\n");
    }
    
    return 0;
}
