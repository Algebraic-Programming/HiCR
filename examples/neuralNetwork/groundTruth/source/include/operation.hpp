#pragma once

#include <string>
#include <unordered_map>
#include <variant>

#include <hicr/core/exceptions.hpp>

using attributes_t    = std::unordered_map<std::string, std::variant<int64_t, float>>;
using operation_t     = class Operation;
using operationsMap_t = std::unordered_map<std::string, operation_t>;

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
    : _attributes(attributes) {};

  /**
   * Return a specific attribute value as a int64_t provided as a string. 
   * Throws exception if the attribute does not exits or is not of type int64_t
   * 
   * @param[in] key the attribute name
   * 
   * @return the attribute value as a int64_t
  */
  __INLINE__ int64_t getIntAttribute(const std::string &key) const
  {
    if (_attributes.contains(key) == false) { HICR_THROW_RUNTIME("The attribute %s does not exist", key.c_str()); }
    const auto &value = _attributes.at(key);
    if (holds_alternative<int64_t>(value) == false) { HICR_THROW_RUNTIME("The attribute %s is not of type int64_t", key.c_str()); }
    return get<int64_t>(value);
  }

  /**
   * Return a specific attribute value as a float provided as a string. 
   * Throws exception if the attribute does not exits or is not of type float 
   * 
   * @param[in] key the attribute name
   * 
   * @return the attribute value as a float 
  */
  __INLINE__ float getFloatAttribute(const std::string &key) const
  {
    if (_attributes.contains(key) == false) { HICR_THROW_RUNTIME("The attribute %s does not exist", key.c_str()); }
    const auto &value = _attributes.at(key);
    if (holds_alternative<float>(value) == false) { HICR_THROW_RUNTIME("The attribute %s is not of type float", key.c_str()); }
    return get<float>(value);
  }

  private:

  /**
   * Collection of operation attributes
  */
  std::unordered_map<std::string, std::variant<int64_t, float>> _attributes;
};
