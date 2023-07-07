#include "cblas.h"
#include "lapack.h"

const char* variantName = "Blocked OpenMP Threads";

// blocked version with omp parallel for on gemm loop
void cholesky(double* __restrict__ A, const size_t n, const size_t bs )
{
 size_t nb = n / bs ;

 // Lapack variables
 char UPLO='L';
 int info=0;
 int nint = n;
 int bsint = bs;

 for( size_t i = 0; i < nb; i++ )
 {
  LAPACK_dpotrf( &UPLO, &bsint, &(A[ (i * bs) * n + i * bs ]), &nint, &info );

  for( size_t j = i + 1; j < nb; j++ )
  {
   cblas_dtrsm (CblasRowMajor, CblasLeft, CblasUpper, CblasTrans, CblasNonUnit, bs, bs, 1.0, &(A[ i * bs * n + i * bs ]), n, &(A[ i * bs * n + j * bs ]), n);

   #pragma omp parallel for
   for( size_t k = i + 1; k < j + 1; k++ )
    if( k != j )  cblas_dgemm(CblasRowMajor, CblasTrans, CblasNoTrans, bs, bs, bs, -1.0, &(A[ i * bs * n + k * bs ]), n, &(A[ i * bs * n + j * bs ]), n, 1.0, &(A[ k * bs * n + j * bs ]), n);
    else          cblas_dsyrk(CblasRowMajor, CblasUpper, CblasTrans, bs, bs, -1.0, &(A[ i * bs * n + k * bs ]), n, 1.0, &(A[ k * bs * n + j * bs ]), n );
  }
 }
}
