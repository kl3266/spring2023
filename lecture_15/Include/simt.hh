#ifndef __SIMT_HH__
#define __SIMT_HH__

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

#endif
