/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file executionUnit.hpp
 * @brief This file implements the abstract execution unit class for the host (CPU) backends
 * @author S. M. Martin
 * @date 9/10/2023
 */

#pragma once

#include <hicr/definitions.hpp>
#include <hicr/L0/executionUnit.hpp>
#include <backends/host/coroutine.hpp>

namespace HiCR
{

namespace backend
{

namespace host
{

namespace L0
{

/**
 * This class represents a replicable C++ executable function for the CPU-based backends
 */
class ExecutionUnit final : public HiCR::L0::ExecutionUnit
{
  public:

  /**
   * Constructor for the execution unit class of the sequential backend
   *
   * \param fc A replicable C++ function to run in this execution unit
   */
  ExecutionUnit(Coroutine::coroutineFc_t fc) : HiCR::L0::ExecutionUnit(), _fc(fc){};
  ExecutionUnit() = delete;

  /**
   * Default destructor
   */
  ~ExecutionUnit() = default;

  __USED__ inline std::string getType() const override { return "C++ Function"; }

  /**
   * Returns the internal function stored inside this execution unit
   *
   * \return The internal function stored inside this execution unit
   */
  __USED__ inline const Coroutine::coroutineFc_t &getFunction() const { return _fc; }

  private:

  /**
   * Replicable internal C++ function to run in this execution unit
   */
  const Coroutine::coroutineFc_t _fc;
};

} // namespace L0

} // namespace host

} // namespace backend

} // namespace HiCR
