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

#include <unordered_set>
#include <hicr/common/definitions.hpp>
#include <hicr/common/exceptions.hpp>
#include <hicr/executionUnit.hpp>
#include <hicr/processingUnit.hpp>

namespace HiCR
{

namespace backend
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
   * Common type for a collection of compute resources
   */
  typedef std::unordered_set<computeResourceId_t> computeResourceList_t;

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
  virtual ExecutionUnit *createExecutionUnit(ExecutionUnit::function_t executionUnit) = 0;

  /**
   * This function prompts the backend to perform the necessary steps to discover and list the compute resources provided by the library which it supports.
   *
   * In case of change in resource availability during runtime, users need to re-run this function to be able to see the changes.
   *
   * \internal It does not return anything because we want to allow users to run only once, and then consult it many times without having to make a copy.
   */
  __USED__ inline void queryComputeResources()
  {
    // Clearing existing compute resources
    _computeResourceList.clear();

    // Calling backend-internal implementation
    _computeResourceList = queryComputeResourcesImpl();
  }

  /**
   * This function returns the list of queried compute resources, as visible by the backend.
   *
   * If this function is called before queryResources, then it will return an empty container.
   *
   * @return The list of compute resources, as detected the last time \a queryResources was executed.
   */
  __USED__ inline const computeResourceList_t getComputeResourceList()
  {
    // Getting value by copy
    const auto value = _computeResourceList;

    return value;
  }

  /**
   * Creates a new processing unit from the provided compute resource
   *
   * \param[in] resource This is the identifier of the compute resource to use to instantiate into a processing unit. The identifier should be one of those provided by the backend. Providing an arbitrary identifier may lead to unexpected behavior.
   *
   * @return A unique pointer to the newly created processing unit. It is important to preserve the uniqueness of this object, since it represents a physical resource (e.g., core) and we do not want to assign it to multiple workers.
   */
  __USED__ inline std::unique_ptr<ProcessingUnit> createProcessingUnit(computeResourceId_t resource)
  {
    // Checking whether the referenced compute resource actually exists
    if (_computeResourceList.contains(resource) == false) HICR_THROW_RUNTIME("Attempting to create processing unit from a compute resource that does not exist (%lu) in this backend", resource);

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
  virtual std::unique_ptr<ProcessingUnit> createProcessingUnitImpl(computeResourceId_t resource) const = 0;

  /**
   * Backend-internal implementation of the queryComputeResources function
   *
   * @return A list of compute resources
   */
  virtual computeResourceList_t queryComputeResourcesImpl() = 0;

  private:

  /**
   * The internal container for the queried compute resources.
   */
  computeResourceList_t _computeResourceList;
};

} // namespace backend

} // namespace HiCR
