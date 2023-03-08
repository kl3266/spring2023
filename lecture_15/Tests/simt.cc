#include<stdint.h>
#include<vector>

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
};

class kernel
{
    private:
	std::vector<u32>	_shape;

    public:
	void operator[](u32 n) { _shape.push_back(n); }
};

template <typename T1>
class	kernel1 : public kernel
{
    public:
	kernel1(void (*f)(T1 arg1));
};

template <typename T1, typename T2>
class	kernel2 : public kernel
{
    public:
	kernel2(void (*f)(T1 arg1, T2 arg2));
	void operator()(T1 arg1, T2 arg2);
	kernel2<T1,T2>& operator[](u32 n) { (*(kernel*)this)[n]; return *this; }
};

template <typename T1, typename T2, typename T3, typename T4, typename T5>
class	kernel5 : public kernel
{
    public:
	kernel5(void (*f)(T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5));
	void operator()(T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5);
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
    u8	*dst;
    u8	*src;

    for (u32 n=1; n<1024; n *= 2)
    {
	Kernel(cpy)[n](dst, src);
    }

    for (u32 m=2; m<64; m *= 2)
    {
	u32 n = 2*m;
	double *y;
	double *A;
	double *x;
	Kernel(vxv)[m](y, A, x, n, m);
    }
    return 0;
}
