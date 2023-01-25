TESTS 	= memcpy mxv
CCC	= g++
CCFLAGS	= -I../Include ../Src/simple.cc
DEPS	= ../Include/simple.hh ../Src/simple.cc

all:	${TESTS}

%: 	%.cc ../Src/%.cc ../Include/%.hh $(DEPS)
	${CCC} ${CCFLAGS} $< ../Src/$< -o $@

clean:
	/bin/rm -rf ${TESTS}
