#pragma once

#include <algorithm>
#include <iterator>
#include <numeric>
#include <unordered_map>
#include <vector>

#include <hicr/core/definitions.hpp>

using tensor_t     = class Tensor;
using tensorsMap_t = std::unordered_map<std::string, tensor_t>;

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
  Tensor(std::vector<uint64_t> shape, std::vector<float> data)
    : _shape(shape),
      _data(data)
  {
    // Convert shape for 1D tensors to 2D
    if (_shape.size() == 1) { _shape.emplace(_shape.begin(), 1); }
  };

  ~Tensor() = default;

  /**
   * @return tensor shape
  */
  __INLINE__ std::vector<uint64_t> &getShape() { return _shape; }

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
  __INLINE__ float *toFloat() { return _data.data(); }

  /**
   * @return const pointer to the beginning of the vector data 
  */
  __INLINE__ const float *toCFloat() const { return _data.data(); }

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

  /**
   * @return index of the max element in the tensor
  */
  __INLINE__ uint64_t indexOfMax() const { return std::distance(cbegin(), std::max_element(cbegin(), cend())); }

  private:

  /**
 * Tensor shape
*/
  std::vector<uint64_t> _shape;

  /**
   * Tensor data
  */
  std::vector<float> _data;
};
