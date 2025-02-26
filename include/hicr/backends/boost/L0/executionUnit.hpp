/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file executionUnit.hpp
 * @brief This file implements the execution unit class for the Boost backend
 * @author S. M. Martin
 * @date 9/10/2023
 */

#pragma once

#include <hicr/core/definitions.hpp>
#include <hicr/core/L0/executionUnit.hpp>
#include <utility>
#include "../coroutine.hpp"

namespace HiCR::backend::boost::L0
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
  ExecutionUnit(Coroutine::coroutineFc_t fc)
    : HiCR::L0::ExecutionUnit(),
      _fc(std::move(fc)){};
  ExecutionUnit() = delete;

  /**
   * Default destructor
   */
  ~ExecutionUnit() override = default;

  [[nodiscard]] __INLINE__ std::string getType() const override { return "Boost C++ Coroutine Function"; }

  /**
   * Returns the internal function stored inside this execution unit
   *
   * \return The internal function stored inside this execution unit
   */
  [[nodiscard]] __INLINE__ const Coroutine::coroutineFc_t &getFunction() const { return _fc; }

  private:

  /**
   * Replicable internal C++ function to run in this execution unit
   */
  const Coroutine::coroutineFc_t _fc;
};

} // namespace HiCR::backend::boost::L0
