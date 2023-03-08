#ifndef _SGEMV_HH_
#define _SGEMV_HH_

namespace pipelined
{
    void sgemv(float *y, float *A, float *x, uint32_t m, uint32_t n, uint32_t lda);
};

#endif
