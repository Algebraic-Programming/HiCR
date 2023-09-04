#include "lapack.h"

const char *variantName = "Unblocked (LAPACK_dpotrf2)";

// lapack version
void cholesky(double *__restrict__ A, const size_t n, const size_t bs)
{
  char UPLO = 'L';
  int info = 0;
  int nint = n;
  LAPACK_dpotrf2(&UPLO, &nint, A, &nint, &info);
}
