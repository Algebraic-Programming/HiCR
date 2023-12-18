/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file computeManager.hpp
 * @brief Provides a definition for the abstract compute manager class
 * @author S. M. Martin
 * @date 27/6/2023
 */

#pragma once

#include <hicr/L0/computeResource.hpp>
#include <hicr/L0/executionUnit.hpp>
#include <hicr/L0/processingUnit.hpp>
#include <hicr/common/definitions.hpp>
#include <hicr/common/exceptions.hpp>

namespace HiCR
{

namespace L1
{

/**
 * This class represents an abstract definition of a compute manager. That is, the set of functions to be implemented by a given backend that allows
 * the discovery of compute resources, the definition of replicable execution units (functions or kernels to run), and the instantiation of
 * execution states, that represent the execution lifetime of an execution unit.
 */
class ComputeManager
{
  public:

  /**
   * Default destructor
   */
  virtual ~ComputeManager() = default;

  /**
   * This function enables the creation of an execution unit.
   *
   * Its default constructor takes a simple function (supported by most backends), but this method can be overriden to support the execution
   * of other replicable heterogeneous kernels (e.g., GPU, NPU, etc).
   *
   * \param[in] executionUnit The replicable function to execute
   * \return Returns a pointer to the newly created execution unit. The user needs to delete it to free up its allocated memory.
   */
  virtual L0::ExecutionUnit *createExecutionUnit(L0::ExecutionUnit::function_t executionUnit) = 0;

  /**
   * Creates a new processing unit from the provided compute resource
   *
   * \param[in] resource This is the identifier of the compute resource to use to instantiate into a processing unit. The identifier should be one of those provided by the backend. Providing an arbitrary identifier may lead to unexpected behavior.
   *
   * @return A unique pointer to the newly created processing unit. It is important to preserve the uniqueness of this object, since it represents a physical resource (e.g., core) and we do not want to assign it to multiple workers.
   */
  __USED__ inline std::unique_ptr<L0::ProcessingUnit> createProcessingUnit(L0::ComputeResource* resource)
  {
    // Getting value by copy
    auto value = createProcessingUnitImpl(resource);

    // Calling internal implementation
    return value;
  }

  protected:

  /**
   * Backend-internal implementation of the createProcessingUnit function
   *
   * \param[in] resource This is the identifier of the compute resource to use to instantiate into a processing unit. The identifier should be one of those provided by the backend. Providing an arbitrary identifier may lead to unexpected behavior.
   *
   * @return A unique pointer to the newly created processing unit. It is important to preserve the uniqueness of this object, since it represents a physical resource (e.g., core) and we do not want to assign it to multiple workers.
   */
  virtual std::unique_ptr<L0::ProcessingUnit> createProcessingUnitImpl(L0::ComputeResource* resource) const = 0;
};

} // namespace L1

} // namespace HiCR
