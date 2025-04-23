#include <acl/acl.h>

#include "tensor.hpp"

using namespace tensor::ascend;

Tensor::Tensor(std::vector<uint64_t> shape, std::shared_ptr<HiCR::LocalMemorySlot> &data)
  : tensor_t(shape, data)
{
  _tensorDescriptor = aclCreateTensorDesc(ACL_FLOAT, _shape.size(), reinterpret_cast<const int64_t *>(_shape.data()), ACL_FORMAT_ND);
  if (_tensorDescriptor == nullptr) { HICR_THROW_RUNTIME("Can not create tensor descriptor"); }
};

Tensor::~Tensor()
{
  if (_tensorDescriptor != nullptr) { aclDestroyTensorDesc(_tensorDescriptor); }
};

std::shared_ptr<tensor_t> Tensor::create(std::vector<uint64_t> &shape, std::shared_ptr<HiCR::LocalMemorySlot> &data)
{
  return std::make_shared<tensor::ascend::Tensor>(shape, data);
}

std::shared_ptr<tensor_t> Tensor::clone(const tensor_t                     &other,
                                        HiCR::MemoryManager                &memoryManager,
                                        std::shared_ptr<HiCR::MemorySpace> &memorySpace,
                                        HiCR::CommunicationManager         &communicationManager)
{
  auto memSlot = memoryManager.allocateLocalMemorySlot(memorySpace, other.getData()->getSize());
  communicationManager.memcpy(memSlot, 0, other.getData(), 0, other.getData()->getSize());
  return std::make_shared<tensor::ascend::Tensor>(other.getShape(), memSlot);
}

std::shared_ptr<HiCR::LocalMemorySlot> Tensor::toHost(HiCR::MemoryManager                      &memoryManager,
                                                      HiCR::CommunicationManager               &communicationManager,
                                                      const std::shared_ptr<HiCR::MemorySpace> &hostMemSpace)
{
  auto dstMemSlot = memoryManager.allocateLocalMemorySlot(hostMemSpace, _data->getSize());
  communicationManager.memcpy(dstMemSlot, 0, _data, 0, _data->getSize());
  return dstMemSlot;
}
