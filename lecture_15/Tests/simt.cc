#include<stdlib.h>
#include<stdint.h>
#include<assert.h>
#include<vector>
#include<iostream>

typedef uint8_t		u8;
typedef uint32_t	u32;

class simt
{
    private:
	static u32	_i;
	static u32	_j;
	static u32	_k;

    public:
	static u32	i() { return _i; }
	static u32&	seti() { return _i; }
};

u32	simt::_i = 0;

class kernel
{
    private:
	std::vector<u32>	_shape;

    public:
	void operator[](u32 n) 		{ _shape.push_back(n); }
	u32 dimensions() const 		{ return _shape.size(); }
	u32 shape(u32 axis) const	{ assert(axis < dimensions()); return _shape[axis]; }
};

template <typename T1>
class	kernel1 : public kernel
{
    private:
	void (*_f)(T1 arg1);
    public:
	kernel1(void (*f)(T1 arg1)) { _f = f; }
};

template <typename T1, typename T2>
class	kernel2 : public kernel
{
    private:
	void (*_f)(T1 arg1, T2 arg2);
    public:
	kernel2(void (*f)(T1 arg1, T2 arg2)) { _f = f; }
	void operator()(T1 arg1, T2 arg2) 
	{ 
	    assert(dimensions() == 1);
	    u32 n = shape(0);
	    for (u32 i=0; i<n; i++)
	    {
		simt::seti() = i;
		_f(arg1, arg2); 
	    }
	}
	kernel2<T1,T2>& operator[](u32 n) { (*(kernel*)this)[n]; return *this; }
};

template <typename T1, typename T2, typename T3, typename T4, typename T5>
class	kernel5 : public kernel
{
    private:
	void (*_f)(T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5);
    public:
	kernel5(void (*f)(T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5)) { _f = f; }
	void operator()(T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5) 
	{ 
	    assert(dimensions() == 1);
	    u32 n = shape(0);
	    for (u32 i=0; i<n; i++)
	    {
		simt::seti() = i;
		_f(arg1, arg2, arg3, arg4, arg5); 
	    }
	}
	kernel5<T1,T2,T3,T4,T5>& operator[](u32 n) { (*(kernel*)this)[n]; return *this; }
};

template <typename T1>
kernel1<T1> Kernel
(
    void (*f)(T1 arg1)
)
{
    return kernel1<T1>(f);
}

template <typename T1, typename T2>
kernel2<T1,T2>	Kernel
(
    void (*f)(T1 arg1, T2 arg2)
)
{
    return kernel2<T1,T2>(f);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5>
kernel5<T1,T2,T3,T4,T5>	Kernel
(
    void (*f)(T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5)
)
{
    return kernel5<T1,T2,T3,T4,T5>(f);
}

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

	std::cout << "n = " << n << " ";

	bool pass = true;
	for (u32 i=0; i<n; i++) if (dst[i] != src[i]) pass - false;
	if (pass) std::cout << " | PASS";
	else      std::cout << " | FAIL";
	std::cout << std::endl;
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

	std::cout << "m = " << m << ", n = " << n << " ";
	bool pass = true;
	for (u32 i=0; i<m; i++) if (y[i] != ((n*(n-1))/2)*i) pass = false;
	if (pass) std::cout << " | PASS";
	else      std::cout << " | FAIL";
	std::cout << std::endl;
    }

    return 0;
}
