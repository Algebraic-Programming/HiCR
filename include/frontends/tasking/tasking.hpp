/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file runtime.hpp
 * @brief This file implements the HiCR tasking initializers
 * @author Sergio Martin
 * @date 3/4/2024
 */

#pragma once

#include "task.hpp"
#include "worker.hpp"
#include "common.hpp"

namespace HiCR
{

namespace tasking
{

/**
 * Initializes the HiCR tasking environment
 *
 * In particular, it initializes the thread-specific storage needed to recover exactly what task/worker is
 * currently in execution.
 */
__USED__ inline void initialize()
{
  if (_isInitialized == true) HICR_THROW_RUNTIME("HiCR Tasking functionality was already initialized");

  // Making sure the task and worker-identifying keys are created
  pthread_key_create(&_workerPointerKey, NULL);
  pthread_key_create(&_taskPointerKey, NULL);

  // Setting initialized flag
  _isInitialized = true;
}

/**
 * Finalizes the common thread-specific structures to free up memory
 *
 * Concurrency issues may arise if any remaining thread try to access the keys here
 * while we finalize. Make sure all HiCR workers are stopped before finalizing
 */
__USED__ inline void finalize()
{
  if (_isInitialized == false) HICR_THROW_RUNTIME("HiCR Tasking functionality was not yet initialized");

  // Setting is initialized to false first.
  _isInitialized = false;

  // Deleting previously created pthread keys
  pthread_key_delete(_workerPointerKey);
  pthread_key_delete(_taskPointerKey);
}

} // namespace tasking

} // namespace HiCR
