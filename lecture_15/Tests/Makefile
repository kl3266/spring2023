TESTS 	= memcpy mxv vmemcpy sgemv simt
CCC	= g++
CCFLAGS	= -g -I../Include ../Src/pipelined.cc
DEPS	= ../Include/pipelined.hh ../Src/pipelined.cc

all:	${TESTS}

%: 	%.cc ../Src/%.cc ../Include/%.hh $(DEPS)
	${CCC} ${CCFLAGS} $< ../Src/$< -o $@

clean:
	/bin/rm -rf ${TESTS}
