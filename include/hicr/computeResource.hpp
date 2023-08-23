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

#include <hicr/common/definitions.hpp>
#include <hicr/common/logger.hpp>
#include <hicr/dispatcher.hpp>

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
  public:
  virtual ~ComputeResource() = default;

  /**
   * Initializes the resource and leaves it ready to execute work
   */
  HICR_API virtual inline void initialize() = 0;

  /**
   * Starts running the resource and execute a user-defined function
   *
   * \param[in] fc The function to execute by the resource
   */
  HICR_API virtual inline void run(resourceFc_t fc) = 0;

  /**
   * Triggers the suspension of the resource. All the elements that make the resource remain active in memory, but will not execute.
   */
  HICR_API virtual inline void suspend() = 0;

  /**
   * Resumes the execution of the resource.
   */
  HICR_API virtual inline void resume() = 0;

  /**
   * Triggers the finalization the execution of the resource. This is an asynchronous operation, so returning from this function does not guarantee that the resource has finalized.
   */
  HICR_API virtual inline void finalize() = 0;

  /**
   * Suspends the execution of the caller until the finalization is ultimately completed
   */
  HICR_API virtual inline void await() = 0;

  /**
   * Copy of the function to be ran by the resource
   */
  resourceFc_t _fc;
};

} // namespace HiCR
