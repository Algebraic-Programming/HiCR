#pragma once

#include <acl/acl.h>
#include <memory>
#include <unordered_set>

#include <hicr/backends/acl/communicationManager.hpp>
#include <hicr/backends/acl/computeManager.hpp>
#include <hicr/backends/acl/memoryManager.hpp>
#include <hicr/backends/acl/computationKernel.hpp>
#include <hicr/backends/acl/kernel.hpp>
#include <hicr/backends/acl/memoryKernel.hpp>

#include "../executionUnitFactory.hpp"

namespace factory::acl
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
  ExecutionUnitFactory(HiCR::backend::acl::ComputeManager       &computeManager,
                       HiCR::backend::acl::CommunicationManager &communicationManager,
                       HiCR::backend::acl::MemoryManager        &memoryManager,
                       std::shared_ptr<HiCR::MemorySpace>       &deviceMemorySpace,
                       std::shared_ptr<HiCR::MemorySpace>       &hostMemorySpace);

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
   * acl compute manager
  */
  HiCR::backend::acl::ComputeManager &_computeManager;

  /**
 * acl communication manager
 */
  HiCR::backend::acl::CommunicationManager &_communicationManager;

  /**
 * acl memory manager
 */
  HiCR::backend::acl::MemoryManager &_memoryManager;

  /**
 * acl device memory space
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

}; // namespace factory::acl
