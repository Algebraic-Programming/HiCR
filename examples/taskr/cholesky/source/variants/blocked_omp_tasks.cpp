#include "cblas.h"
#include "lapack.h"

const char *variantName = "Blocked OpenMP Tasks";

// blocked version with omp tasks dependencies
void cholesky(double *__restrict__ A, const size_t n, const size_t bs)
{
  size_t nb = n / bs;

  // LAPACK input output parameters
  int info = 0;
  int nint = n;
  int bsint = bs;

#pragma omp parallel
#pragma omp single
  for (size_t i = 0; i < nb; i++)
  {
#pragma omp task depend(inout : A[(i * bs) * n + (i * bs)]) priority((nb - i) * 2)
    LAPACK_dpotrf("L", &bsint, &(A[(i * bs) * n + i * bs]), &nint, &info);

    for (size_t j = i + 1; j < nb; j++)
#pragma omp task depend(inout : A[(i * bs) * n + (j * bs)]) depend(in : A[(i * bs) * n + (i * bs)]) priority((nb - j) * 2)
      cblas_dtrsm(CblasRowMajor, CblasLeft, CblasUpper, CblasTrans, CblasNonUnit, bs, bs, 1.0, &(A[i * bs * n + i * bs]), n, &(A[i * bs * n + j * bs]), n);

    for (size_t j = i + 1; j < nb; j++)
      for (size_t k = i + 1; k < j + 1; k++)
        if (k != j)
        {
#pragma omp task depend(inout : A[(k * bs) * n + (j * bs)]) depend(in : A[(i * bs) * n + (k * bs)]) depend(in : A[(i * bs) * n + (j * bs)]) priority(nb - j + nb - k)
          cblas_dgemm(CblasRowMajor, CblasTrans, CblasNoTrans, bs, bs, bs, -1.0, &(A[i * bs * n + k * bs]), n, &(A[i * bs * n + j * bs]), n, 1.0, &(A[k * bs * n + j * bs]), n);
        }
        else
        {
#pragma omp task depend(inout : A[(k * bs) * n + (j * bs)]) depend(in : A[(i * bs) * n + (k * bs)]) priority(nb - j + nb - k)
          cblas_dsyrk(CblasRowMajor, CblasUpper, CblasTrans, bs, bs, -1.0, &(A[i * bs * n + k * bs]), n, 1.0, &(A[k * bs * n + j * bs]), n);
        }
  }
}
