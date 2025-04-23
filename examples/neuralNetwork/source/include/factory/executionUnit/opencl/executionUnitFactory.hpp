#pragma once

#include <CL/opencl.hpp>
#include <memory>

#include <hicr/backends/opencl/communicationManager.hpp>
#include <hicr/backends/opencl/computeManager.hpp>
#include <hicr/backends/opencl/memoryManager.hpp>
#include <hicr/backends/opencl/computationKernel.hpp>
#include <hicr/backends/opencl/kernel.hpp>
#include <hicr/backends/opencl/memoryKernel.hpp>

#include "../executionUnitFactory.hpp"

namespace factory::opencl
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
  ExecutionUnitFactory(HiCR::backend::opencl::ComputeManager       &computeManager,
                       HiCR::backend::opencl::CommunicationManager &communicationManager,
                       HiCR::backend::opencl::MemoryManager        &memoryManager,
                       std::shared_ptr<HiCR::MemorySpace>          &deviceMemorySpace,
                       std::shared_ptr<HiCR::MemorySpace>          &hostMemorySpace,
                       cl::Program                                 &program)
    : _computeManager(computeManager),
      _communicationManager(communicationManager),
      _memoryManager(memoryManager),
      _deviceMemorySpace(deviceMemorySpace),
      _hostMemorySpace(hostMemorySpace),
      _program(program)
  {}

  ~ExecutionUnitFactory() = default;

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
   * OpenCL compute manager
  */
  HiCR::backend::opencl::ComputeManager &_computeManager;

  /**
 * OpenCL communication manager
 */
  HiCR::backend::opencl::CommunicationManager &_communicationManager;

  /**
 * OpenCL memory manager
 */
  HiCR::backend::opencl::MemoryManager &_memoryManager;

  /**
 * OpenCL device memory space
 */
  std::shared_ptr<HiCR::MemorySpace> &_deviceMemorySpace;

  /**
 * Host memory space
 */
  std::shared_ptr<HiCR::MemorySpace> &_hostMemorySpace;

  /**
   * OpenCL program
   */
  cl::Program &_program;
};

}; // namespace factory::opencl
