#include<simple.hh>
#include<memcpy.hh>
#include<stdio.h>

int main //how is this file related to the memcpy.cc at Src folder?
(
    int		  argc,
    char	**argv
)
{
    simple::zeromem();
    const uint32_t N = 1024;
    for (uint32_t i=0; i<N; i++) simple::MEM[i] = rand() % 0xff; simple::MEM_h[i] = rand() % 0xff;
    for (uint32_t n = 1; n<=N; n *= 2)
    {
	simple::GPR[3] = 1024;		//*dest
	simple::GPR[4] = 0;			//*src
	simple::GPR[5] = n;			//size_t n
	
	simple::zeroctrs();
	simple::memcpy(0,0,0);		//what does this do?
	
	printf("n = %6d : instructions = %6lu, cycles = %6lu :", n, simple::instructions, simple::cycles);
	bool pass = true;
	for (uint32_t i=0; i<n; i++) if (simple::MEM[i] != simple::MEM[N+i]) pass = false;
	if (pass) printf("PASS\n");
	else      printf("FAIL\n");
    }
    return 0;
}
