/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file executeState.hpp
 * @brief Provides a base definition for a HiCR ExecutionState class
 * @author S. M. Martin
 * @date 13/10/2023
 */
#pragma once

#include <hicr/executionUnit.hpp>

namespace HiCR
{

class ExecutionState
{
  public:

 virtual void resume() = 0;
 virtual void yield() = 0;

   ~ExecutionState() = default;

  protected:

  ExecutionState(const ExecutionUnit* executionUnit)
  {

  };

  private:

  /**
   *  Task context preserved as a coroutine
   */
  common::Coroutine _coroutine;
};

} // namespace HiCR
