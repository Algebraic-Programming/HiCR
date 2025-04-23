#pragma once

#include <memory>
#include <vector>

#include <hicr/core/localMemorySlot.hpp>
#include <hicr/core/memorySpace.hpp>
#include <hicr/core/communicationManager.hpp>
#include <hicr/core/memoryManager.hpp>

#include "../tensor.hpp"

namespace tensor::pthreads
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
    : tensor_t(shape, data) {};

  ~Tensor() = default;

  static std::shared_ptr<tensor_t> create(std::vector<uint64_t> &shape, std::shared_ptr<HiCR::LocalMemorySlot> &data);

  static std::shared_ptr<tensor_t> clone(const tensor_t                         &other,
                                         HiCR::MemoryManager                &memoryManager,
                                         std::shared_ptr<HiCR::MemorySpace> &memorySpace,
                                         HiCR::CommunicationManager         &communicationManager);

  /**
   * @return begin iterator
  */
  __INLINE__ float *begin() { return toFloat(); }

  /**
   * @return end iterator
  */
  __INLINE__ float *end() { return toFloat() + size(); }

  /**
   * @return const begin iterator
  */
  __INLINE__ const float *cbegin() const { return toCFloat(); }

  /**
   * @return const end iterator
  */
  __INLINE__ const float *cend() const { return toCFloat() + size(); }

  /**
   * @return non-const pointer to the beginning of the vector data 
  */
  __INLINE__ float *toFloat() { return (float *)_data->getPointer(); }

  /**
   * @return const pointer to the beginning of the vector data 
  */
  __INLINE__ const float *toCFloat() const { return (const float *)_data->getPointer(); }
};
}; // namespace tensor::pthreads