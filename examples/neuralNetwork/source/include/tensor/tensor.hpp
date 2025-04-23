#pragma once

#include <algorithm>
#include <iterator>
#include <numeric>
#include <unordered_map>
#include <vector>

#include <hicr/core/definitions.hpp>
#include <hicr/core/localMemorySlot.hpp>
#include <hicr/core/memorySpace.hpp>
#include <hicr/core/communicationManager.hpp>
#include <hicr/core/memoryManager.hpp>

namespace tensor
{

/**
 * Class representing a 1D or 2D Tensor.
*/
class Tensor
{
  public:

  /**
   * Constructor for a tensor float
   * 
   * @param[in] shape tensor shape
   * @param[in] data tensor data
  */
  Tensor(std::vector<uint64_t> shape, std::shared_ptr<HiCR::LocalMemorySlot> &data)
    : _shape(shape),
      _data(data)
  {
    // Convert shape for 1D tensors to 2D
    if (_shape.size() == 1) { _shape.emplace(_shape.begin(), 1); }
  };

  virtual ~Tensor() = default;

  /**
   * @return tensor shape
  */
  __INLINE__ const std::vector<uint64_t> &getShape() const { return _shape; }

  /**
   * @return memory slot
   */
  __INLINE__ const std::shared_ptr<HiCR::LocalMemorySlot> &getData() const { return _data; }

  /**
   * @return number of tensor rows 
  */
  __INLINE__ uint64_t rows() const { return _shape[0]; }

  /**
   * @return number of tensor columns
  */

  __INLINE__ uint64_t columns() const { return _shape[1]; }
  /**
   * @return number of elements in the tensor
  */
  __INLINE__ uint64_t size() const { return std::accumulate(_shape.begin(), _shape.end(), 1, std::multiplies<>()); }

  protected:

  /**
   * Tensor shape
  */
  std::vector<uint64_t> _shape;

  /**
   * Tensor data
  */
  std::shared_ptr<HiCR::LocalMemorySlot> _data;
};

}; // namespace tensor

using tensor_t          = tensor::Tensor;
using tensorFactoryFc_t = std::function<std::shared_ptr<tensor_t>(std::vector<uint64_t> &, std::shared_ptr<HiCR::LocalMemorySlot> &)>;
using tensorCloneFc_t =
  std::function<std::shared_ptr<tensor_t>(const tensor_t &, HiCR::MemoryManager &, std::shared_ptr<HiCR::MemorySpace> &, HiCR::CommunicationManager &)>;
using tensorsMap_t = std::unordered_map<std::string, std::shared_ptr<tensor_t>>;
