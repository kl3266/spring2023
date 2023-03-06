#ifndef _MXV_HH_
#define _MXV_HH_

namespace pipelined
{
    void spmv(double *y, uint32_t nnz, uint32_t *i, uint32_t *j, double *a, double *x);
};

#endif
