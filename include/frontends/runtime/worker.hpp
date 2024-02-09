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
#include "channel.hpp"

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


  Worker() = delete;
  Worker(const std::string& entryPoint,
         std::shared_ptr<HiCR::L0::Instance> hicrInstance
        ) :
   _entryPoint(entryPoint),
   _hicrInstance(hicrInstance) {}
  ~Worker() = default;

  inline std::shared_ptr<HiCR::L0::Instance> getInstance() const { return _hicrInstance; }
  inline std::string getEntryPoint() const { return _entryPoint; }

  private:

  /**
   * Initial entry point function name being executed by the worker
   */ 
  const std::string _entryPoint;

  /**
   * Internal HiCR instance class
   */ 
  const std::shared_ptr<HiCR::L0::Instance> _hicrInstance;

  /**
   * Consumer channel to receive RPCs from the coordinator instance
   */ 
  const std::unique_ptr<runtime::Channel> _rpcInputChannel;
};

} // namespace runtime

} // namespace HiCR
