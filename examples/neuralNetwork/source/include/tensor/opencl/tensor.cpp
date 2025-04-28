#include <CL/opencl.hpp>

#include "tensor.hpp"

using namespace tensor::opencl;

std::shared_ptr<tensor_t> Tensor::create(std::vector<uint64_t> &shape, std::shared_ptr<HiCR::LocalMemorySlot> &data)
{
  return std::make_shared<tensor::opencl::Tensor>(shape, data);
}

std::shared_ptr<tensor_t> Tensor::clone(const tensor_t                     &other,
                                        HiCR::MemoryManager                &memoryManager,
                                        std::shared_ptr<HiCR::MemorySpace> &memorySpace,
                                        HiCR::CommunicationManager         &communicationManager)
{
  auto memSlot = memoryManager.allocateLocalMemorySlot(memorySpace, other.getData()->getSize());
  communicationManager.memcpy(memSlot, 0, other.getData(), 0, other.getData()->getSize());
  return std::make_shared<tensor::opencl::Tensor>(other.getShape(), memSlot);
}