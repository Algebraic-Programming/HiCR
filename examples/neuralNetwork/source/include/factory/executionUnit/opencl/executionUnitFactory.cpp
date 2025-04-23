#include <algorithm>
#include <vector>

#include "../../../tensor/opencl/tensor.hpp"
#include "executionUnitFactory.hpp"

using namespace factory::opencl;

std::shared_ptr<HiCR::ExecutionUnit> ExecutionUnitFactory::gemm(const gemmArgs_t &args)
{
  auto                                                M = args.A->rows();
  auto                                                N = args.B->rows();
  auto                                                K = args.B->columns();
  std::vector<std::shared_ptr<HiCR::LocalMemorySlot>> kernelArguments({args.A->getData(), args.B->getData(), args.C->getData()});

  auto kernel = std::make_shared<cl::Kernel>(_program, "gemm_kernel");
  kernel->setArg<unsigned int>(3, M);
  kernel->setArg<unsigned int>(4, N);
  kernel->setArg<unsigned int>(5, K);
  kernel->setArg<const float>(6, args.alpha);
  kernel->setArg<const float>(7, args.beta);
  kernel->setArg<const int>(8, args.transposeB);
  auto global     = cl::NDRange(M, N);
  auto gemmKernel = std::make_shared<HiCR::backend::opencl::ComputationKernel>(kernel, std::move(kernelArguments), cl::NullRange, global, cl::NullRange);

  return _computeManager.createExecutionUnit(std::vector<std::shared_ptr<HiCR::backend::opencl::Kernel>>{gemmKernel});
}

std::shared_ptr<HiCR::ExecutionUnit> ExecutionUnitFactory::relu(const reluArgs_t &args)
{
  std::vector<std::shared_ptr<HiCR::LocalMemorySlot>> kernelArguments({args.t->getData(), args.t->getData()});
  auto                                                kernel = std::make_shared<cl::Kernel>(_program, "relu_kernel");
  kernel->setArg<const int>(2, args.t->size());
  auto global     = cl::NDRange(args.t->size());
  auto reluKernel = std::make_shared<HiCR::backend::opencl::ComputationKernel>(kernel, std::move(kernelArguments), cl::NullRange, global, cl::NullRange);
  return _computeManager.createExecutionUnit(std::vector<std::shared_ptr<HiCR::backend::opencl::Kernel>>{reluKernel});
}

std::shared_ptr<HiCR::ExecutionUnit> ExecutionUnitFactory::vectorAdd(const vectorAddArgs_t &args)
{
  std::vector<std::shared_ptr<HiCR::LocalMemorySlot>> kernelArguments({args.a->getData(), args.b->getData(), args.a->getData()});
  auto                                                kernel = std::make_shared<cl::Kernel>(_program, "vector_add_kernel");
  kernel->setArg<const int>(3, args.a->size());
  auto global          = cl::NDRange(args.a->size());
  auto vectorAddKernel = std::make_shared<HiCR::backend::opencl::ComputationKernel>(kernel, std::move(kernelArguments), cl::NullRange, global, cl::NullRange);
  return _computeManager.createExecutionUnit(std::vector<std::shared_ptr<HiCR::backend::opencl::Kernel>>{vectorAddKernel});
}
