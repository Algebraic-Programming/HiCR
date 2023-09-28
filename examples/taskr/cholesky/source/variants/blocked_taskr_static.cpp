#include "cblas.h"
#include "lapack.h"
#include <taskr.hpp>
#include <hicr/backends/sharedMemory/sharedMemory.hpp>

const char *variantName = "Blocked Taskr (Static)";

inline uint64_t getLabelPotrf(const int i, const int nb) { return 2 * nb * nb * nb + i; }
inline uint64_t getLabelTrsm(const int i, const int j, const int nb) { return 1 * nb * nb * nb + i * nb + j; }
inline uint64_t getLabelGemm(const int i, const int j, const int k, const int nb) { return 0 * nb * nb * nb + i * nb * nb + j * nb + k; }

inline void blockedPotrf(double *__restrict__ A, const size_t n, const size_t bs, const int i)
{
  int info = 0;
  int nint = n;
  int bsint = bs;

  LAPACK_dpotrf("L", &bsint, &(A[(i * bs) * n + i * bs]), &nint, &info);
}

inline void blockedTrsm(double *__restrict__ A, const size_t n, const size_t bs, const int i, const int j)
{
  cblas_dtrsm(CblasRowMajor, CblasLeft, CblasUpper, CblasTrans, CblasNonUnit, bs, bs, 1.0, &(A[i * bs * n + i * bs]), n, &(A[i * bs * n + j * bs]), n);
}

inline void blockedGemm(double *__restrict__ A, const size_t n, const size_t bs, const int i, const int j, const int k)
{
  if (k != j) cblas_dgemm(CblasRowMajor, CblasTrans, CblasNoTrans, bs, bs, bs, -1.0, &(A[i * bs * n + k * bs]), n, &(A[i * bs * n + j * bs]), n, 1.0, &(A[k * bs * n + j * bs]), n);
  if (k == j) cblas_dsyrk(CblasRowMajor, CblasUpper, CblasTrans, bs, bs, -1.0, &(A[i * bs * n + k * bs]), n, 1.0, &(A[k * bs * n + j * bs]), n);
}

// blocked version with omp tasks dependencies
void cholesky(double *__restrict__ A, const size_t n, const size_t bs)
{
  // Initializing Pthreads backend to run in parallel
  auto t = new HiCR::backend::sharedMemory::SharedMemory();

  // Initializing taskr
  taskr::initialize(t);

  size_t nb = n / bs;

  for (size_t i = 0; i < nb; i++)
  {
    auto potrfTask = new taskr::Task(getLabelPotrf(i, nb), [A, n, bs, i]() { blockedPotrf(A, n, bs, i); });
    if (i > 0) potrfTask->addTaskDependency(getLabelGemm(i - 1, i, i, nb));
    taskr::addTask(potrfTask);

    for (size_t j = i + 1; j < nb; j++)
    {
      auto trsmTask = new taskr::Task(getLabelTrsm(i, j, nb), [A, n, bs, i, j]() { blockedTrsm(A, n, bs, i, j); });
      trsmTask->addTaskDependency(getLabelPotrf(i, nb));
      if (i > 0) trsmTask->addTaskDependency(getLabelGemm(i - 1, j, i, nb));
      taskr::addTask(trsmTask);
    }

    for (size_t j = i + 1; j < nb; j++)
      for (size_t k = i + 1; k < j + 1; k++)
      {
        auto gemmTask = new taskr::Task(getLabelGemm(i, j, k, nb), [A, n, bs, i, j, k]() { blockedGemm(A, n, bs, i, j, k); });
        gemmTask->addTaskDependency(getLabelTrsm(i, j, nb));
        gemmTask->addTaskDependency(getLabelTrsm(i, k, nb));
        taskr::addTask(gemmTask);
      }
  }

  // Running taskr
  taskr::run();

  // Finalizing taskr
  taskr::finalize();
}
