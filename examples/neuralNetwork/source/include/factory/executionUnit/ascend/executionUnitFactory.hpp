#pragma once

#include <acl/acl.h>
#include <memory>
#include <unordered_set>

#include <hicr/backends/ascend/communicationManager.hpp>
#include <hicr/backends/ascend/computeManager.hpp>
#include <hicr/backends/ascend/memoryManager.hpp>
#include <hicr/backends/ascend/computationKernel.hpp>
#include <hicr/backends/ascend/kernel.hpp>
#include <hicr/backends/ascend/memoryKernel.hpp>

#include "../executionUnitFactory.hpp"

namespace factory::ascend
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
  ExecutionUnitFactory(HiCR::backend::ascend::ComputeManager       &computeManager,
                       HiCR::backend::ascend::CommunicationManager &communicationManager,
                       HiCR::backend::ascend::MemoryManager        &memoryManager,
                       std::shared_ptr<HiCR::MemorySpace>          &deviceMemorySpace,
                       std::shared_ptr<HiCR::MemorySpace>          &hostMemorySpace);

  ~ExecutionUnitFactory();

  /**
   * Create execution unit for GEMM operation
   * 
   * @param[in] args GEMM arguments
   * 
   * @return gemm execution unit
  */
  std::shared_ptr<HiCR::ExecutionUnit> gemm(const gemmArgs_t &args) override; 

  /**
   * Create execution unit for relu operation
   *    
   * @param[in] args ReLU arguments
   * 
   * @return relu execution unit
  */
  std::shared_ptr<HiCR::ExecutionUnit> relu(const reluArgs_t &args) override;

  /**
   * Create execution unit for vector add operation
   * 
   * @param[in] args vector add arguments
   * 
   * @return vector add execution unit
  */
  std::shared_ptr<HiCR::ExecutionUnit> vectorAdd(const vectorAddArgs_t &args) override;

  private:

  /**
   * Ascend compute manager
  */
  HiCR::backend::ascend::ComputeManager &_computeManager;

  /**
 * Ascend communication manager
 */
  HiCR::backend::ascend::CommunicationManager &_communicationManager;

  /**
 * Ascend memory manager
 */
  HiCR::backend::ascend::MemoryManager &_memoryManager;

  /**
 * Ascend device memory space
 */
  std::shared_ptr<HiCR::MemorySpace> &_deviceMemorySpace;

  /**
 * Host memory space
 */
  std::shared_ptr<HiCR::MemorySpace> &_hostMemorySpace;

  /**
 * Default empty kernel attributes
 */
  aclopAttr *_emptyKernelAttributes;

  /**
   * Kernel attributes created with GEMMs
   */
  std::unordered_set<aclopAttr *> _kernelAttributes;

  /**
   * Tensor descriptors created with GEMMs
   */
  std::unordered_set<aclTensorDesc *> _alphaBetaTensorDescriptors;
};

}; // namespace factory::ascend
