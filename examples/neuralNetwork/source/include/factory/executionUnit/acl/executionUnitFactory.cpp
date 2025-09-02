#include "../../../tensor/acl/tensor.hpp"
#include "executionUnitFactory.hpp"

using namespace factory::acl;

ExecutionUnitFactory::ExecutionUnitFactory(HiCR::backend::acl::ComputeManager       &computeManager,
                                           HiCR::backend::acl::CommunicationManager &communicationManager,
                                           HiCR::backend::acl::MemoryManager        &memoryManager,
                                           std::shared_ptr<HiCR::MemorySpace>       &deviceMemorySpace,
                                           std::shared_ptr<HiCR::MemorySpace>       &hostMemorySpace)
  : _computeManager(computeManager),
    _communicationManager(communicationManager),
    _memoryManager(memoryManager),
    _deviceMemorySpace(deviceMemorySpace),
    _hostMemorySpace(hostMemorySpace)
{
  _emptyKernelAttributes = aclopCreateAttr();
  if (_emptyKernelAttributes == nullptr) { HICR_THROW_RUNTIME("Can not create kernel attributes"); }
}

ExecutionUnitFactory::~ExecutionUnitFactory()
{
  for (auto &tensorDescriptor : _alphaBetaTensorDescriptors) { aclDestroyTensorDesc(tensorDescriptor); }
  for (auto &kernelAttribute : _kernelAttributes) { aclopDestroyAttr(kernelAttribute); }
  aclopDestroyAttr(_emptyKernelAttributes);
}

std::shared_ptr<HiCR::ExecutionUnit> ExecutionUnitFactory::gemm(const gemmArgs_t &args)
{
  auto alphaValue = args.alpha;
  auto betaValue  = args.beta;
  auto transposeB = args.transposeB;

  const auto A = dynamic_pointer_cast<tensor::acl::Tensor>(args.A);
  if (A == nullptr) { HICR_THROW_RUNTIME("Can not downcast tensor to the supported type"); }
  const auto B = dynamic_pointer_cast<tensor::acl::Tensor>(args.B);
  if (B == nullptr) { HICR_THROW_RUNTIME("Can not downcast tensor to the supported type"); }
  auto C = dynamic_pointer_cast<tensor::acl::Tensor>(args.C);
  if (C == nullptr) { HICR_THROW_RUNTIME("Can not downcast tensor to the supported type"); }

  const int64_t alphaBetaDimensions[]     = {1};
  auto          alphaBetaTensorDescriptor = aclCreateTensorDesc(ACL_FLOAT, 1, alphaBetaDimensions, ACL_FORMAT_ND);
  if (alphaBetaTensorDescriptor == nullptr) { HICR_THROW_RUNTIME("Can not create alpha and beta tensor descriptor"); }
  _alphaBetaTensorDescriptors.insert(alphaBetaTensorDescriptor);

  auto alphaHostMemSlot   = _memoryManager.registerLocalMemorySlot(_hostMemorySpace, (void *)&alphaValue, sizeof(alphaValue));
  auto betaHostMemSlot    = _memoryManager.registerLocalMemorySlot(_hostMemorySpace, (void *)&betaValue, sizeof(betaValue));
  auto alphaDeviceMemSlot = _memoryManager.allocateLocalMemorySlot(_deviceMemorySpace, sizeof(alphaValue));
  auto betaDeviceMemSlot  = _memoryManager.allocateLocalMemorySlot(_deviceMemorySpace, sizeof(betaValue));

  _communicationManager.memcpy(alphaDeviceMemSlot, 0, alphaHostMemSlot, 0, alphaHostMemSlot->getSize());
  _communicationManager.memcpy(betaDeviceMemSlot, 0, betaHostMemSlot, 0, betaHostMemSlot->getSize());

  _memoryManager.deregisterLocalMemorySlot(alphaHostMemSlot);
  _memoryManager.deregisterLocalMemorySlot(betaHostMemSlot);

  auto inputs =
    std::vector<HiCR::backend::acl::ComputationKernel::tensorData_t>{HiCR::backend::acl::ComputationKernel::createTensorData(A->getData(), A->getTensorDescriptor()),
                                                                     HiCR::backend::acl::ComputationKernel::createTensorData(B->getData(), B->getTensorDescriptor()),
                                                                     HiCR::backend::acl::ComputationKernel::createTensorData(C->getData(), C->getTensorDescriptor()),
                                                                     HiCR::backend::acl::ComputationKernel::createTensorData(alphaDeviceMemSlot, alphaBetaTensorDescriptor),
                                                                     HiCR::backend::acl::ComputationKernel::createTensorData(betaDeviceMemSlot, alphaBetaTensorDescriptor)};
  auto outputs = std::vector<HiCR::backend::acl::ComputationKernel::tensorData_t>{HiCR::backend::acl::ComputationKernel::createTensorData(C->getData(), C->getTensorDescriptor())};

  auto gemmKernelAttributes = aclopCreateAttr();
  if (gemmKernelAttributes == nullptr) { HICR_THROW_RUNTIME("Can not create GEMM kernel attributes"); }
  aclopSetAttrBool(gemmKernelAttributes, "transpose_a", 0);
  aclopSetAttrBool(gemmKernelAttributes, "transpose_b", transposeB);
  _kernelAttributes.insert(gemmKernelAttributes);

  auto gemmKernel = std::make_shared<HiCR::backend::acl::ComputationKernel>("GEMM", std::move(inputs), std::move(outputs), gemmKernelAttributes);
  return _computeManager.createExecutionUnit(std::vector<std::shared_ptr<HiCR::backend::acl::Kernel>>{gemmKernel});
}

std::shared_ptr<HiCR::ExecutionUnit> ExecutionUnitFactory::relu(const reluArgs_t &args)
{
  auto t = dynamic_pointer_cast<tensor::acl::Tensor>(args.t);
  if (t == nullptr) { HICR_THROW_RUNTIME("Can not downcast tensor to the supported type"); }
  auto &shape = t->getShape();

  auto tensorDescriptor = aclCreateTensorDesc(ACL_FLOAT, shape.size(), reinterpret_cast<const int64_t *>(shape.data()), ACL_FORMAT_ND);
  _alphaBetaTensorDescriptors.insert(tensorDescriptor);
  auto tensorData = HiCR::backend::acl::ComputationKernel::createTensorData(t->getData(), tensorDescriptor);
  auto inputs     = std::vector<HiCR::backend::acl::ComputationKernel::tensorData_t>{tensorData};
  auto outputs    = std::vector<HiCR::backend::acl::ComputationKernel::tensorData_t>{tensorData};
  auto reluKernel = std::make_shared<HiCR::backend::acl::ComputationKernel>("Relu", std::move(inputs), std::move(outputs), _emptyKernelAttributes);
  return _computeManager.createExecutionUnit(std::vector<std::shared_ptr<HiCR::backend::acl::Kernel>>{reluKernel});
}

std::shared_ptr<HiCR::ExecutionUnit> ExecutionUnitFactory::vectorAdd(const vectorAddArgs_t &args)
{
  auto a = dynamic_pointer_cast<tensor::acl::Tensor>(args.a);
  if (a == nullptr) { HICR_THROW_RUNTIME("Can not downcast tensor to the supported type"); }
  auto b = dynamic_pointer_cast<tensor::acl::Tensor>(args.b);
  if (b == nullptr) { HICR_THROW_RUNTIME("Can not downcast tensor to the supported type"); }
  auto &shape = a->getShape();

  auto tensorDescriptor = aclCreateTensorDesc(ACL_FLOAT, shape.size(), reinterpret_cast<const int64_t *>(shape.data()), ACL_FORMAT_ND);
  _alphaBetaTensorDescriptors.insert(tensorDescriptor);
  auto aTensorData     = HiCR::backend::acl::ComputationKernel::createTensorData(a->getData(), tensorDescriptor);
  auto bTensorData     = HiCR::backend::acl::ComputationKernel::createTensorData(b->getData(), tensorDescriptor);
  auto inputs          = std::vector<HiCR::backend::acl::ComputationKernel::tensorData_t>{aTensorData, bTensorData};
  auto outputs         = std::vector<HiCR::backend::acl::ComputationKernel::tensorData_t>{aTensorData};
  auto vectorAddKernel = std::make_shared<HiCR::backend::acl::ComputationKernel>("Add", std::move(inputs), std::move(outputs), _emptyKernelAttributes);
  return _computeManager.createExecutionUnit(std::vector<std::shared_ptr<HiCR::backend::acl::Kernel>>{vectorAddKernel});
}
