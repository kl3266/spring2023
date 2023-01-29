#ifndef _ISA_HH_
#define _ISA_HH_

#include<pipelined.hh>

// 1. Branch Facility
#define b(X)			if (instructions::b::execute(0)) goto X;
#define beq(X)			if (instructions::beq::execute(0)) goto X;

// 2. Fixed-point Facility

// 2.1. Load/Store instructions
#define lbz(RT, RA)		instructions::lbz::execute(RT, RA)
#define stb(RS, RA)		instructions::stb::execute(RS, RA)

// 2.2. Arithmetic instructions
#define addi(RT, RA, SI)	instructions::addi::execute(RT, RA, SI)

// 2.3. Compare instructions
#define cmpi(RA, SI)		instructions::cmpi::execute(RA, SI)

// 3. Floating-point Facility

// 3.1 Load/Store instructions
#define lfd(FT, RA)		instructions::lfd::execute(FT, RA)
#define stfd(FS, RA)		instructions::stfd::execute(FS, RA)

// 3.2. Arithmetic instructions
#define zd(FT)			instructions::zd::execute(FT)
#define fmul(FT, FA, FB)	instructions::fmul::execute(FT, FA, FB)
#define fadd(FT, FA, FB)	instructions::fadd::execute(FT, FA, FB)

#endif
