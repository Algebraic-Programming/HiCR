#pragma once

#include <cblas.h>

#include <hicr/backends/pthreads/computeManager.hpp>

#include "../executionUnitFactory.hpp"

namespace factory::pthreads
{
/**
 * Factory class to create execution unit for each of the network operation
*/
class ExecutionUnitFactory final : public factory::ExecutionUnitFactory
{
  public:

  /**
   * Constructor
   * 
   * @param[in] computeManager a HiCR compute manager
   *
  */
  ExecutionUnitFactory(HiCR::backend::pthreads::ComputeManager &computeManager)
    : _computeManager(computeManager)
  {}

  /**
   * Create execution unit for gemm operation
   * 
   * @return gemm execution unit
  */
  std::shared_ptr<HiCR::ExecutionUnit> gemm(const gemmArgs_t &args) override;

  /**
   * Create execution unit for relu operation
   * 
   * @return relu execution unit
  */
  std::shared_ptr<HiCR::ExecutionUnit> relu(const reluArgs_t &args) override;

  /**
   * Create execution unit for vector add operation
   * 
   * @return vector add execution unit
  */
  std::shared_ptr<HiCR::ExecutionUnit> vectorAdd(const vectorAddArgs_t &args) override;

  private:

  /**
   * Pthreads compute manager
  */
  HiCR::backend::pthreads::ComputeManager &_computeManager;
};

}; // namespace factory::pthreads
