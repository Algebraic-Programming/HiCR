/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file worker.hpp
 * @brief Provides a definition for the Runtime worker class.
 * @author S. M. Martin
 * @date 5/2/2024
 */

#pragma once

#include <memory>
#include <hicr/definitions.hpp>
#include <hicr/L0/instance.hpp>

namespace HiCR
{

namespace runtime
{

/**
 * Defines the worker class, which represents a self-contained instance of HiCR with access to compute and memory resources.
 *
 * Workers may be created during runtime (if the process managing backend allows for it), or activated/suspended on demand.
 */
class Worker final
{
  public:

  /**
   * Type definition for a worker id
  */
  typedef uint64_t workerId_t; 

  Worker() = delete;
  Worker(const workerId_t id, const std::string& task, std::shared_ptr<HiCR::L0::Instance> hicrInstance) :
   _id(id),
   _task(task),
   _hicrInstance(hicrInstance) {}
  ~Worker() = default;

  inline std::shared_ptr<HiCR::L0::Instance> getInstance() const { return _hicrInstance; }

  private:

  // Unique identifier for the worker given the current deployment
  const workerId_t _id;

  // Initial task being executed by the worker
  const std::string _task;

  // Internal HiCR instance class
  const std::shared_ptr<HiCR::L0::Instance> _hicrInstance;
};

} // namespace runtime

} // namespace HiCR
