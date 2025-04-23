#include <cblas.h>

#include "../../../tensor/pthreads/tensor.hpp"
#include "executionUnitFactory.hpp"

std::shared_ptr<HiCR::ExecutionUnit> factory::pthreads::ExecutionUnitFactory::gemm(const gemmArgs_t &args)
{
  return _computeManager.createExecutionUnit([=](void *) {
    // Get attributes
    auto alpha      = args.alpha;
    auto beta       = args.beta;
    auto transposeB = args.transposeB;

    const auto A = dynamic_pointer_cast<tensor::pthreads::Tensor>(args.A);
    if (A == nullptr) { HICR_THROW_RUNTIME("Can not downcast tensor to the supported type"); }
    const auto B = dynamic_pointer_cast<tensor::pthreads::Tensor>(args.B);
    if (B == nullptr) { HICR_THROW_RUNTIME("Can not downcast tensor to the supported type"); }
    auto C = dynamic_pointer_cast<tensor::pthreads::Tensor>(args.C);
    if (C == nullptr) { HICR_THROW_RUNTIME("Can not downcast tensor to the supported type"); }

    // Define M N K
    blasint M = A->rows();
    blasint N = B->rows();
    blasint K = B->columns();

    // Define leading dimension and whether B should be transposed
    blasint lda = K;
    blasint ldc = N;

    blasint         ldb;
    CBLAS_TRANSPOSE cblasTransposeB;
    if (transposeB == true)
    {
      cblasTransposeB = CblasTrans;
      ldb             = K;
    }
    else
    {
      cblasTransposeB = CblasNoTrans;
      ldb             = N;
    }

    // Perform the GEMM
    cblas_sgemm(CblasRowMajor, CblasNoTrans, cblasTransposeB, M, N, K, alpha, A->toCFloat(), lda, B->toCFloat(), ldb, beta, C->toFloat(), ldc);
  });
}

std::shared_ptr<HiCR::ExecutionUnit> factory::pthreads::ExecutionUnitFactory::relu(const reluArgs_t &args)
{
  return _computeManager.createExecutionUnit([=](void *) {
    auto t = dynamic_pointer_cast<tensor::pthreads::Tensor>(args.t);
    if (t == nullptr) { HICR_THROW_RUNTIME("Can not downcast tensor to the supported type"); }

    auto data = t->toFloat();
    for (uint64_t i = 0; i < t->size(); i++) { data[i] = std::max(0.0f, data[i]); }
  });
}

std::shared_ptr<HiCR::ExecutionUnit> factory::pthreads::ExecutionUnitFactory::vectorAdd(const vectorAddArgs_t &args)
{
  return _computeManager.createExecutionUnit([=](void *) {
    auto a = dynamic_pointer_cast<tensor::pthreads::Tensor>(args.a);
    if (a == nullptr) { HICR_THROW_RUNTIME("Can not downcast tensor to the supported type"); }

    auto b = dynamic_pointer_cast<tensor::pthreads::Tensor>(args.b);
    if (b == nullptr) { HICR_THROW_RUNTIME("Can not downcast tensor to the supported type"); }

    std::transform(a->begin(), a->end(), b->cbegin(), a->begin(), std::plus<float>());
  });
}