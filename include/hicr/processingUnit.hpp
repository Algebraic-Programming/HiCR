/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file processingUnit.hpp
 * @brief Provides a definition for a HiCR ProcessingUnit class
 * @author S. M. Martin
 * @date 13/7/2023
 */
#pragma once

#include <memory>
#include <set>

#include <hicr/common/definitions.hpp>
#include <hicr/common/logger.hpp>
#include <hicr/dispatcher.hpp>

namespace HiCR
{

/**
 * Type definition for a generic memory space identifier
 */
typedef uint64_t computeResourceId_t;

/**
 * Definition for function to run at resource
 */
typedef std::function<void(void)> processingUnitFc_t;

/**
 * This class represents an abstract definition for a Processing Unit resource in HiCR that:
 *
 * - Represents a single computational resource that has been instantiated for execution (as opposed of those who shall remain unused or unassigned).
 * - Is capable of executing or contributing to the execution of tasks.
 * - Is assigned to a worker to perform the work necessary to execute a task.
 */
class ProcessingUnit
{
  private:

  /**
   * Identifier of the compute resource associated to this processing unit
   */
  computeResourceId_t _computeResourceId;

  public:

  /**
   * Disabled default constructor
   */
  ProcessingUnit() = delete;

  /**
   * A processing unit is created to instantiate a single compute resource
   *
   * \param computeResourceId The identifier of the compute resource to instantiate, as indicated by the backend
   */
  __USED__ inline ProcessingUnit(computeResourceId_t computeResourceId) : _computeResourceId(computeResourceId){};

  virtual ~ProcessingUnit() = default;

  /**
   * Initializes the resource and leaves it ready to execute work
   */
  virtual void initialize() = 0;

  /**
   * Starts running the resource and execute a user-defined function
   *
   * @param[in] fc The function to execute by the resource
   */
  virtual void run(processingUnitFc_t fc) = 0;

  /**
   * Triggers the suspension of the resource. All the elements that make the resource remain active in memory, but will not execute.
   */
  virtual void suspend() = 0;

  /**
   * Resumes the execution of the resource.
   */
  virtual void resume() = 0;

  /**
   * Triggers the finalization the execution of the resource. This is an asynchronous operation, so returning from this function does not guarantee that the resource has finalized.
   */
  virtual void finalize() = 0;

  /**
   * Suspends the execution of the caller until the finalization is ultimately completed
   */
  virtual void await() = 0;

  /**
   * Returns the processing unit's associated compute resource
   *
   * \return The identifier of the compute resource associated to this processing unit.
   */
  __USED__ inline computeResourceId_t getComputeResourceId() { return _computeResourceId; }

  /**
   * Copy of the function to be ran by the processing unit
   */
  processingUnitFc_t _fc;
};

} // namespace HiCR
