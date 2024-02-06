/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file instance.hpp
 * @brief Provides a definition for the Runtime Instance class.
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
 * Defines the instance class, which represents a self-contained instance of HiCR with access to compute and memory resources.
 *
 * Instances may be created during runtime (if the process managing backend allows for it), or activated/suspended on demand.
 */
class Instance final
{
  public:

  Instance() = default;
  ~Instance() = default;

  private:

  // Internal HiCR instance class
  std::unique_ptr<HiCR::L0::Instance> _hicrInstance;
};

} // namespace runtime

} // namespace HiCR
