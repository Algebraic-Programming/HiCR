#include <cstring>
#include <cstdio>
#include "cblas.h"
#include "timer.hpp"
#include "utils.hpp"
#include "argparse/argparse.hpp"

#define MEM_ALIGN 4096

#ifdef _C_CHOLESKY
extern "C" void cholesky(double* __restrict__ A, const size_t n, const size_t bs);
#else
extern void cholesky(double* __restrict__ A, const size_t n, const size_t bs);
#endif

extern const char* variantName;

int main( int argc, char** argv )
{
 // Parsing command line arguments
 argparse::ArgumentParser program("cholesky", "1.0");
 program.add_argument("n").help("Matrix Size").required();
 program.add_argument("bs").help("Block Size").required();
 program.add_argument("--check").help("Enables residual calculation").default_value(false).implicit_value(true);
 try { program.parse_args(argc, argv);  }
 catch (const std::runtime_error &err) { fprintf(stderr, "[Error] %s\n%s", err.what(), program.help().str().c_str()); }

 size_t n = std::stoul(program.get<std::string>("n"));
 size_t bs = std::stoul(program.get<std::string>("bs"));
 bool doCheck = program.get<bool>("--check");

 printf("Running Variant '%s' with N = %lu and BS = %lu\n", variantName, n, bs);
 if (n % bs != 0) { fprintf(stderr, "[Error] Block size does not divide matrix size evenly\n"); exit(-1); }

 double *A    = (double*) aligned_alloc (MEM_ALIGN, n * n * sizeof(double));
 double *Atmp = (double*) aligned_alloc (MEM_ALIGN, n * n * sizeof(double));
 double *L    = (double*) aligned_alloc (MEM_ALIGN, n * n * sizeof(double));

 grb::utils::Timer timer;

 printf("Initializing Matrices...\n"); fflush(stdout);
 generate_matrix( A, L, Atmp,  n );

 printf("Starting computation...\n"); fflush(stdout);
 timer.reset();
 cholesky(L, n, bs);
 printf("Compute Time: %.3fs\n", timer.time() / 1000.0);

 if (doCheck)
 {
  printf("Calculating residual...\n"); fflush(stdout);
  double residual = calculateResidual(L, Atmp, n);
  printf("Residual: %g\n", residual);
 }

 free(A);
 free(Atmp);
 free(L);

 return 0;
}
