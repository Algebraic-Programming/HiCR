#pragma once

#include <string>
#include <unordered_map>
#include <variant>

#include <hicr/core/exceptions.hpp>

namespace operation
{

using attributes_t = std::unordered_map<std::string, std::variant<int64_t, float>>;

/**
 * Concept to restrict the types that can be retrieved to the ones in the variant
 */
template <typename T>
concept AttributeType = std::same_as<T, int64_t> || std::same_as<T, float>;
/**
 * Class to represent an operation in the graph.
 * It holds the pre-trained data of the operation like weights and bias for GEMM
 * */
class Operation
{
  public:

  /**
   * Constructor for an operation
   * 
   * @param[in] attributes the set of attributes of the operation
  */
  Operation(attributes_t &attributes)
    : _attributes(attributes){};

  /**
   * Return a specific attribute value provided as a string. 
   * Throws exception if the attribute does not exits or if the type specified is incorrect
   * 
   * @param[in] key the attribute name
   * 
   * @return the attribute value as a T
  */
  template <AttributeType T>
  __INLINE__ T getAttribute(const std::string &key) const
  {
    if (_attributes.contains(key) == false) { HICR_THROW_RUNTIME("The attribute %s does not exist", key.c_str()); }
    const auto &value = _attributes.at(key);
    if (holds_alternative<T>(value) == false) { HICR_THROW_RUNTIME("The attribute %s is not of the desired type", key.c_str()); }
    return get<T>(value);
  }

  private:

  /**
   * Collection of operation attributes
  */
  attributes_t _attributes;
};

}; // namespace operation

using operation_t     = operation::Operation;
using operationsMap_t = std::unordered_map<std::string, operation_t>;