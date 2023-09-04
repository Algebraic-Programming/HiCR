#pragma once

#include <iostream>
#include <math.h>
#include <random>

inline void generate_matrix(double *A, double *L, double *Atmp, const size_t n)
{
  // srand(0);
  size_t i = 0, j = 0;

  // std::random_device rd;  // Will be used to obtain a seed for the random number engine
  // std::mt19937 gen(rd()); // Standard mersenne_twister_engine seeded with rd()
  // std::uniform_real_distribution<> dis(0.0, 1.0);
  //
  // #pragma omp parallel for
  // for (i = 0; i < n; i++) {
  //  for (j = i; j < n; j++) {
  //   A[ i * n + j ] =  dis(gen);
  //   if( i == j ) {
  //    A[i * n + j] = A[i * n + j] + n * n;
  //   } else {
  //    A[j * n + i] = A[i * n + j];
  //   }
  //  }
  // }

#pragma omp parallel for
  for (i = 0; i < n * n; i++)
  {
    A[i] = 1.0;
    L[i] = 1.0;
    Atmp[i] = 1.0;
  }

#pragma omp parallel for
  for (j = 0; j < n; j++)
  {
    A[j + j * n] = n;
    L[j + j * n] = n;
    Atmp[j + j * n] = n;
  }
}

inline void show_matrix(const double *A, const size_t n)
{
  std::cout << "\n";
  size_t i = 0, j = 0;
  for (i = 0; i < n; i++)
  {
    for (j = 0; j < n; j++)
    {
      if (std::abs(A[i * n + j]) > 1.e-10)
      {
        std::cout << A[i * n + j] << " ";
      }
      else
      {
        std::cout << 0. << " ";
      }
    }
    std::cout << "\n";
  }
  std::cout << "\n";
}

inline double calculateResidual(double *__restrict__ L, double *__restrict__ A, const size_t n)
{
#pragma omp parallel for
  for (size_t i = 0; i < n; i++)
    for (size_t j = 0; j < i; j++) L[i * n + j] = 0;
  cblas_dgemm(CblasRowMajor, CblasTrans, CblasNoTrans, n, n, n, -1.0, L, n, L, n, 1.0, A, n);

  double sum = 0;
#pragma omp parallel for reduction(+ : sum)
  for (size_t i = 0; i < n * n; i++) sum += A[i] * A[i];

  return std::sqrt(sum);
}
