/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file backend.hpp
 * @brief Provides a definition for the base backend compute manager class.
 * @author S. M. Martin
 * @date 27/6/2023
 */

#pragma once

#include <mutex>
#include <set>

#include <hicr/common/definitions.hpp>
#include <hicr/common/exceptions.hpp>
#include <hicr/processingUnit.hpp>
#include <hicr/executionUnit.hpp>

namespace HiCR
{

namespace backend
{

class ComputeManager
{
  public:

  /**
   * Common definition of a collection of compute resources
   */
  typedef std::set<computeResourceId_t> computeResourceList_t;

  virtual ~ComputeManager() = default;

  virtual ExecutionUnit* createExecutionUnit(ExecutionUnit::function_t executionUnit) = 0;

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
  __USED__ inline ProcessingUnit* createProcessingUnit(computeResourceId_t resource)
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
  virtual ProcessingUnit* createProcessingUnitImpl(computeResourceId_t resource) const = 0;

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
