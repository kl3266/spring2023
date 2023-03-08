#include<pipelined.hh>
#include<spmv.hh>
#include<stdio.h>

using namespace pipelined;

void test_spmv(u32 m, u32 n)
{
    pipelined::zeromem();

    const uint32_t M = m;
    const uint32_t N = n;
    const uint32_t NNZ = 10;
    const uint32_t Y = 0;
    const uint32_t X = Y + M*sizeof(double);
    const uint32_t A = X + N*sizeof(double);
    const uint32_t I = A + NNZ*sizeof(double);
    const uint32_t J = I + NNZ*sizeof(uint32_t);
    double a_col[10] = {0,5,2,4,1,8,3,7,6,9};
    uint32_t i_col[10] = {0,3,1,2,0,5,1,4,3,5};
    uint32_t j_col[10] = {0,0,1,1,2,2,3,3,4,4};
    double a_row[10] = {0,1,2,3,4,5,6,7,8,9};
    uint32_t i_row[10] = {0,0,1,1,2,3,3,4,5,5};
    uint32_t j_row[10] = {0,2,1,3,1,0,4,3,2,4};
    bool row = false;   // control row major order or column major order

    for (uint32_t i=0; i<M; i++) *((double*)(pipelined::MEM.data() + Y + i*sizeof(double))) = 0.0;  // set Y to 0
    for (uint32_t j=0; j<N; j++) *((double*)(pipelined::MEM.data() + X + j*sizeof(double))) = (double) j;   // populate x
    if (row)
    {
        for (uint32_t k=0; k<NNZ; k++) *((double*)(pipelined::MEM.data() + A + k*sizeof(double))) = a_row[k]; // populate A row major order
        for (uint32_t k=0; k<NNZ; k++) *((uint32_t*)(pipelined::MEM.data() + I + k*sizeof(uint32_t))) = i_row[k]; // populate I row major order
        for (uint32_t k=0; k<NNZ; k++) *((uint32_t*)(pipelined::MEM.data() + J + k*sizeof(uint32_t))) = j_row[k]; // populate J row major order      
    }
    else
    {
        for (uint32_t k=0; k<NNZ; k++) *((double*)(pipelined::MEM.data() + A + k*sizeof(double))) = a_col[k]; // populate A column major order
        for (uint32_t k=0; k<NNZ; k++) *((uint32_t*)(pipelined::MEM.data() + I + k*sizeof(uint32_t))) = i_col[k]; // populate I column major order
        for (uint32_t k=0; k<NNZ; k++) *((uint32_t*)(pipelined::MEM.data() + J + k*sizeof(uint32_t))) = j_col[k]; // populate J column major order    
    }

    pipelined::zeroctrs();

    pipelined::GPR[3].data() = Y;
    pipelined::GPR[4].data() = NNZ;
    pipelined::GPR[5].data() = I;
    pipelined::GPR[6].data() = J;
    pipelined::GPR[7].data() = A;
    pipelined::GPR[8].data() = X;
    
    pipelined::spmv(0,0,0,0,0,0);

    pipelined::caches::L2.flush();
    pipelined::caches::L3.flush();
    
    if (pipelined::tracing) printf("\n");
    printf("M = %4d, N = %4d : instr = %6lu, cyc = %8lu, L1D(access= %6lu, hit = %6lu, miss = %6lu), L2(miss = %6lu), L3(miss = %6lu) | ",
	    M, N, pipelined::counters::operations, pipelined::counters::cycles, pipelined::caches::L1D.accesses, pipelined::caches::L1D.hits, pipelined::caches::L1D.misses,
	    pipelined::caches::L2.misses, pipelined::caches::L3.misses);
    bool pass = true;
    for (uint32_t i=0; i<M; i++)
    {
	double y = *((double*)(pipelined::MEM.data() + Y + i*sizeof(double)));
	// printf("\ny[%2d] = %10.0f", i, y);
	//if (y != ((N*(N-1))/2)*i) { pass = false; }
    if (i==0)
        if (y != 2) pass = false;
    if (i==1)
        if (y != 11) pass = false;
    if (i==2)
        if (y != 4) pass = false;
    if (i==3)
        if (y != 24) pass = false;
    if (i==4)
        if (y != 21) pass = false;
    if (i==5)
        if (y != 52) pass = false;        
    }
    // printf("\n");
    if (pass) printf("PASS\n");
    else      printf("FAIL\n");
}

int main
(
    int		  argc,
    char	**argv
)
{
    printf("L1D: %u bytes of capacity, %u sets, %u-way set associative, %u-byte line size\n",
	   pipelined::caches::L1D.capacity(), pipelined::caches::L1D.nsets(), pipelined::caches::L1D.nways(), pipelined::caches::L1D.linesize());
    printf("L1I: %u bytes of capacity, %u sets, %u-way set associative, %u-byte line size\n",
	   pipelined::caches::L1I.capacity(), pipelined::caches::L1I.nsets(), pipelined::caches::L1I.nways(), pipelined::caches::L1I.linesize());
    printf("L2: %u bytes of capacity, %u sets, %u-way set associative, %u-byte line size\n",
	   pipelined::caches::L2.capacity(), pipelined::caches::L2.nsets(), pipelined::caches::L2.nways(), pipelined::caches::L2.linesize());
    printf("L3: %u bytes of capacity, %u sets, %u-way set associative, %u-byte line size\n",
	   pipelined::caches::L3.capacity(), pipelined::caches::L3.nsets(), pipelined::caches::L3.nways(), pipelined::caches::L3.linesize());

    /*for (uint32_t m = 2; m <= 64; m *= 2) for (uint32_t n = m/2; n <= m; n *= 2)
    {
	test_mxv(m,n);
    }
    
    for (uint32_t m = 2; m <= 4; m *= 2) for (uint32_t n = m/2; n <= 1024; n *= 2)
    {
	test_mxv(m,n);
    }*/
    test_spmv(6,5);
    
    return 0;
}
