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

#include <memory>
#include <hicr/core/definitions.hpp>
#include <hicr/core/exceptions.hpp>
#include <hicr/core/L0/computeResource.hpp>
#include <hicr/core/L0/executionUnit.hpp>
#include <hicr/core/L0/executionState.hpp>
#include <hicr/core/L0/processingUnit.hpp>

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
   * Creates a new processing unit from the provided compute resource
   *
   * \param[in] resource This is the identifier of the compute resource to use to instantiate into a processing unit. The identifier should be one of those provided by the backend. Providing an arbitrary identifier may lead to unexpected behavior.
   *
   * @return A unique pointer to the newly created processing unit. It is important to preserve the uniqueness of this object, since it represents a physical resource (e.g., core) and we do not want to assign it to multiple workers.
   */
  virtual std::unique_ptr<HiCR::L0::ProcessingUnit> createProcessingUnit(std::shared_ptr<HiCR::L0::ComputeResource> resource) const = 0;

  /**
   * This function enables the creation of an empty execution state object.
   *
   * The instantiation of its internal memory structures is delayed until explicit initialization to reduce memory usage when, for example, scheduling many tasks that do not need to execute at the same time.
   *
   * \param[in] executionUnit The replicable state-less execution unit to instantiate into an execution state
   * \param[in] argument Argument (closure) to pass to the execution unit to make this execution state unique
   * \return A unique pointer to the newly create execution state. It needs to be unique because the state cannot be simultaneously executed my multiple processing units
   */
  virtual std::unique_ptr<HiCR::L0::ExecutionState> createExecutionState(std::shared_ptr<HiCR::L0::ExecutionUnit> executionUnit, void *const argument = nullptr) const = 0;
};

} // namespace L1

} // namespace HiCR
