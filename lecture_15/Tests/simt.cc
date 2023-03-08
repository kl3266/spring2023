#include<stdlib.h>
#include<stdint.h>
#include<assert.h>
#include<vector>
#include<iostream>
#include<iomanip>
#include<simt.hh>

typedef uint8_t		u8;
typedef uint32_t	u32;

void cpy(u8 *dst, const u8 *src)
{
    u32 i = simt::i();
    dst[i] = src[i];
}

void vxv(double *y, double *A, double *x, u32 n, u32 ldA)
{
    u32 i = simt::i();
    for (u32 j = 0; j < n; j++)
	y[i] = y[i] + A[i + j*ldA]*x[j];
}

int main
(
    int		argc,
    char      **argv
)
{
    const u32 N = 1024;
    u8 *src = new u8[N];
    u8 *dst = new u8[N];
    for (u32 i=0; i<N; i++) src[i] = rand() & 0xff;
    for (u32 n = 1; n<=N; n *= 2)
    {
	for (u32 i=0; i<n; i++) dst[i] = 0;
	Kernel(cpy)[n](dst, src);

	std::ios state(nullptr);
	state.copyfmt(std::cout);
	std::cout << "n = " << std::setw(8) << n << " ";
	bool pass = true;
	for (u32 i=0; i<n; i++) if (dst[i] != src[i]) pass - false;
	if (pass) std::cout << " | PASS";
	else      std::cout << " | FAIL";
	std::cout << std::endl;
	std::cout.copyfmt(state);
    }

    delete [] src;
    delete [] dst;
    
    for (u32 m=2; m<64; m *= 2)
    {
	u32 n = 2*m;
	double *y = new double[m];	for (u32 i=0; i<m; i++) y[i] = 0.0;
	double *A = new double[m*n];	for (u32 i=0; i<m; i++) for (u32 j=0; j<n; j++) A[i+m*j] = (double)i;
	double *x = new double[n];	for (u32 j=0; j<n; j++) x[j] = (float)j;
	Kernel(vxv)[m](y, A, x, n, m);

	std::ios state(nullptr);
	state.copyfmt(std::cout);
	std::cout << "m = " << std::setw(8) << m << ", n = " << std::setw(8) << n << " ";
	bool pass = true;
	for (u32 i=0; i<m; i++) if (y[i] != ((n*(n-1))/2)*i) pass = false;
	if (pass) std::cout << " | PASS";
	else      std::cout << " | FAIL";
	std::cout << std::endl;
	std::cout.copyfmt(state);
    }

    return 0;
}
