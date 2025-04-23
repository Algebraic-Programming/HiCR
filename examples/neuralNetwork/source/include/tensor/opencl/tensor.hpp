#include "../tensor.hpp"

namespace tensor::opencl
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
  Tensor(std::vector<uint64_t> shape, std::shared_ptr<HiCR::LocalMemorySlot> &data)
    : tensor_t(shape, data){};

  ~Tensor() = default;

  static std::shared_ptr<tensor_t> create(std::vector<uint64_t> &shape, std::shared_ptr<HiCR::LocalMemorySlot> &data);

  static std::shared_ptr<tensor_t> clone(const tensor_t                     &other,
                                         HiCR::MemoryManager                &memoryManager,
                                         std::shared_ptr<HiCR::MemorySpace> &memorySpace,
                                         HiCR::CommunicationManager         &communicationManager);
};
}; // namespace tensor::opencl