#include "cblas.h"
#include "lapack.h"
#include <taskr.hpp>

const char* variantName = "Blocked Taskr (Dynamic)";

inline uint64_t getLabelSyrk(const int i, const int j, const int nb)              { return 3*nb*nb*nb + i*nb + j; }
inline uint64_t getLabelPotrf(const int i, const int nb)                          { return 2*nb*nb*nb + i; }
inline uint64_t getLabelTrsm(const int i, const int j, const int nb)              { return 1*nb*nb*nb + i*nb + j; }
inline uint64_t getLabelGemm(const int i, const int j, const int k, const int nb) { return 0*nb*nb*nb + i*nb*nb + j*nb + k; }

inline void blockedPotrf(double* __restrict__ A, const size_t n, const size_t bs, const int i)
{
 int info = 0;
 int nint = n;
 int bsint = bs;

 LAPACK_dpotrf("L", &bsint, &(A[ (i * bs) * n + i * bs ]), &nint, &info );
}

inline void blockedTrsm(double* __restrict__ A, const size_t n, const size_t bs, const int i, const int j)
{
 cblas_dtrsm(CblasRowMajor, CblasLeft, CblasUpper, CblasTrans, CblasNonUnit, bs, bs, 1.0, &(A[ i * bs * n + i * bs ]), n, &(A[ i * bs * n + j * bs ]), n);
}

inline void blockedGemm(double* __restrict__ A, const size_t n, const size_t bs, const int i, const int j, const int k)
{
 cblas_dgemm(CblasRowMajor, CblasTrans, CblasNoTrans, bs, bs, bs, -1.0, &(A[ i * bs * n + k * bs ]), n,      &(A[ i * bs * n + j * bs ]), n, 1.0, &(A[ k * bs * n + j * bs ]), n );
}

inline void blockedSyrk(double* __restrict__ A, const size_t n, const size_t bs, const int i, const int j)
{
 cblas_dsyrk(CblasRowMajor, CblasUpper, CblasTrans,   bs, bs,     -1.0, &(A[ i * bs * n + j * bs ]), n, 1.0, &(A[ j * bs * n + j * bs ]), n );
}

inline void schedulePotrf(double* __restrict__ A, const size_t n, const size_t bs, const size_t nb, const int i)
{
 blockedPotrf(A, n, bs, i);

 // If not finished, create next task
 size_t nextI = i+1;
 if (nextI < nb)
 {
  auto potrfTask = new taskr::Task(getLabelPotrf(nextI, nb), [A, n, bs, nb, nextI] () { schedulePotrf(A, n, bs, nb, nextI); });
  potrfTask->addTaskDependency(getLabelSyrk(i, nextI, nb));
  taskr::addTask(potrfTask);
 }
}

inline void scheduleTrsm(double* __restrict__ A, const size_t n, const size_t bs, const size_t nb, const int i, const int j)
{
 blockedTrsm(A, n, bs, i, j);

 // If not finished, create next task
 size_t nextI = i+1;
 if (nextI < nb && j > i)
 {
  auto trsmTask = new taskr::Task(getLabelTrsm(nextI, j, nb), [A, n, bs, nb, nextI, j] () { scheduleTrsm(A, n, bs, nb, nextI, j); });
  trsmTask->addTaskDependency(getLabelPotrf(nextI, nb));
  trsmTask->addTaskDependency(getLabelGemm(i, j, nextI, nb));
  taskr::addTask(trsmTask);
 }
}

inline void scheduleGemm(double* __restrict__ A, const size_t n, const size_t bs, const size_t nb, const int i, const int j, const int k)
{
 blockedGemm(A, n, bs, i, j, k);

 // If not finished, create next task
 size_t nextI = i+1;
 size_t nextJ = j+1;
 size_t nextK = k+1;
 if (nextI < nb && nextJ < nb && nextK < nb)
 {
  auto gemmTask = new taskr::Task(getLabelGemm(nextI, nextJ, nextK, nb), [A, n, bs, nb, nextI, nextJ, nextK] () { scheduleGemm(A, n, bs, nb, nextI, nextJ, nextK); });
  gemmTask->addTaskDependency(getLabelTrsm(nextI, nextJ, nb));
  gemmTask->addTaskDependency(getLabelTrsm(nextI, nextK, nb));
  taskr::addTask(gemmTask);
 }
}

inline void scheduleSyrk(double* __restrict__ A, const size_t n, const size_t bs, const size_t nb, const int i, const int j)
{
 blockedSyrk(A, n, bs, i, j);

 // If not finished, create next task
 size_t nextI = i+1;
 size_t nextJ = j+1;
 if (nextI < nb && nextJ < nb)
 {
  auto syrkTask = new taskr::Task(getLabelSyrk(nextI, nextJ, nb), [A, n, bs, nb, nextI, nextJ] () { scheduleSyrk(A, n, bs, nb, nextI, nextJ); });
  syrkTask->addTaskDependency(getLabelTrsm(nextI, nextJ, nb));
  taskr::addTask(syrkTask);
 }
}

// blocked version with omp tasks dependencies
void cholesky(double* __restrict__ A, const size_t n, const size_t bs)
{
 size_t nb = n / bs;

 // Initializing taskr
 taskr::initialize();

 auto potrfTask = new taskr::Task(getLabelPotrf(0, nb), [A, n, bs, nb] () { schedulePotrf(A, n, bs, nb, 0); });
 taskr::addTask(potrfTask);

 for( size_t j = 1; j < nb; j++ )
 {
  auto dtrsmTask = new taskr::Task(getLabelTrsm(0, j, nb), [A, n, bs, nb, j] () { scheduleTrsm(A, n, bs, nb, 0, j); });
  dtrsmTask->addTaskDependency(getLabelPotrf(0, nb));
  taskr::addTask(dtrsmTask);
 }

 for( size_t j = 1; j < nb; j++ )
  for( size_t k = 1; k < j + 1; k++ )
  {
   if (j != k)
   {
    auto gemmTask = new taskr::Task(getLabelGemm(0, j, k, nb), [A, n, bs, nb, j, k] () { scheduleGemm(A, n, bs, nb, 0, j, k); });
    gemmTask->addTaskDependency(getLabelTrsm(0, j, nb));
    gemmTask->addTaskDependency(getLabelTrsm(0, k, nb));
    taskr::addTask(gemmTask);
   }
   else
   {
    auto syrkTask = new taskr::Task(getLabelSyrk(0, j, nb), [A, n, bs, nb, j] () { scheduleSyrk(A, n, bs, nb, 0, j); });
    syrkTask->addTaskDependency(getLabelTrsm(0, j, nb));
    taskr::addTask(syrkTask);
   }
  }

 // Running taskr
 taskr::run();

 // Finalizing taskr
 taskr::finalize();
}
