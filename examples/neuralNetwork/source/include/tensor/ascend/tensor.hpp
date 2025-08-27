#include <acl/acl.h>

#include "../tensor.hpp"

namespace tensor::acl
{
/**
 * Class representing a 1D or 2D Tensor.
*/
class Tensor : public tensor_t
{
  public:

  /**
   * Constructor for a tensor float
   * 
   * @param[in] shape tensor shape
   * @param[in] data tensor data
  */
  Tensor(std::vector<uint64_t> shape, std::shared_ptr<HiCR::LocalMemorySlot> &data);

  ~Tensor();

  static std::shared_ptr<tensor_t> create(std::vector<uint64_t> &shape, std::shared_ptr<HiCR::LocalMemorySlot> &data);

  static std::shared_ptr<tensor_t> clone(const tensor_t                     &other,
                                         HiCR::MemoryManager                &memoryManager,
                                         std::shared_ptr<HiCR::MemorySpace> &memorySpace,
                                         HiCR::CommunicationManager         &communicationManager);

  __INLINE__ aclTensorDesc *getTensorDescriptor() { return _tensorDescriptor; }

  std::shared_ptr<HiCR::LocalMemorySlot> toHost(HiCR::MemoryManager                      &memoryManager,
                                                HiCR::CommunicationManager               &communicationManager,
                                                const std::shared_ptr<HiCR::MemorySpace> &hostMemSpace);

  private:

  aclTensorDesc *_tensorDescriptor = nullptr;
};
}; // namespace tensor::acl