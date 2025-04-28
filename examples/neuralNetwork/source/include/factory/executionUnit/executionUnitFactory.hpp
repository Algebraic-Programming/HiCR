#pragma once

#include <memory>
#include <unordered_map>

#include <hicr/core/definitions.hpp>
#include <hicr/core/executionUnit.hpp>

#include "../../arguments.hpp"

using executioUnits_t = std::unordered_map<std::string, std::shared_ptr<HiCR::ExecutionUnit>>;

namespace factory
{
/**
 * Factory class to create execution unit for each of the network operation
*/
class ExecutionUnitFactory
{
  public:

  virtual ~ExecutionUnitFactory() = default;

  /**
   * Create execution unit for gemm operation
   * 
   * @return gemm execution unit
  */
  virtual std::shared_ptr<HiCR::ExecutionUnit> gemm(const gemmArgs_t &args) = 0;

  /**
   * Create execution unit for ReLU operation
   * 
   * @return ReLU execution unit
  */
  virtual std::shared_ptr<HiCR::ExecutionUnit> relu(const reluArgs_t &args) = 0;

  /**
   * Create execution unit for vector add operation
   * 
   * @return vector add execution unit
  */
  virtual std::shared_ptr<HiCR::ExecutionUnit> vectorAdd(const vectorAddArgs_t &args) = 0;
};
} // namespace factory