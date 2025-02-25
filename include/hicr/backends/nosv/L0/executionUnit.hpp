/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file executionUnit.hpp
 * @brief nOS-V execution unit class. Main job is to store the function call
 * @author N. Baumann
 * @date 24/02/2025
 */
#pragma once

#include <hicr/backends/pthreads/L0/executionUnit.hpp>

namespace HiCR::backend::nosv::L0
{

/**
 * This class represents an abstract definition for a Execution Unit that:
 *
 * - Represents a single state-less computational operation (e.g., a function or a kernel) that can be replicated many times
 * - Is executed in the context of an ExecutionState class
 */
class ExecutionUnit final : public HiCR::backend::pthreads::L0::ExecutionUnit
{
  public:

  /**
   * Constructor
   * 
   * \param fc A replicable C++ function to run in this execution unit
   */
  ExecutionUnit(const std::function<void(void *)> &fc)
    : HiCR::backend::pthreads::L0::ExecutionUnit(fc)
  {}
};

} // namespace HiCR::backend::nosv::L0
