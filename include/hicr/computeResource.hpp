/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file computeResource.hpp
 * @brief Provides a definition for a HiCR ComputeResource class
 * @author S. M. Martin
 * @date 13/7/2023
 */
#pragma once

#include <memory>
#include <set>

#include <hicr/common/logger.hpp>
#include <hicr/dispatcher.hpp>

class Worker;

namespace HiCR
{

/**
 * Definition for function to run at resource
 */
typedef std::function<void(void)> resourceFc_t;

/**
 * This class represents an abstract definition for a computational resource in HiCR. Computational resources are assigned to workers to perform the work necessary to execute a task.
 */
class ComputeResource
{
  friend class Worker;

  public:
  virtual ~ComputeResource() = default;

  protected:
  /**
   * Constructor for the resource object. It requires a backend-unique identifier.
   *
   * \param[in] id Identifier for the resource
   */
  Resource(const resourceId_t id) : _id(id){};

  /**
   * Initializes the resource and leaves it ready to execute work
   */
  virtual void initialize() = 0;

  /**
   * Starts running the resource and execute a user-defined function
   *
   * \param[in] fc The function to execute by the resource
   */
  virtual void run(resourceFc_t fc) = 0;

  /**
   * Triggers the finalization the execution of the resource. This is an asynchronous operation, so returning from this function does not guarantee that the resource has finalized.
   */
  virtual void finalize() = 0;

  /**
   * Suspends the execution of the caller until the finalization is ultimately completed
   */
  virtual void await() = 0;

  /**
   * Copy of the function to be ran by the resource
   */
  resourceFc_t _fc;
};

/**
 * Common definition of a collection of resources
 */
typedef std::vector<std::unique_ptr<ComputeResource>> resourceList_t;

} // namespace HiCR
