/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file memorySlot.hpp
 * @brief Provides a n abstractdefinition for all HiCR Memory Slot classes
 * @author S. M. Martin
 * @date 10/1/2024
 */
#pragma once

#include <cstddef>
#include <hicr/definitions.hpp>

namespace HiCR
{

namespace L0
{

/**
 * This class represents an abstract definition for a Memory Slot resource in HiCR that:
 *
 * - Represents a contiguous segment of memory
 * - Contains counters for received and sent messages
 */
class MemorySlot
{
  public:

  protected:

  /**
   * Protected constructor for a MemorySlot class
   */
  MemorySlot() = default;

  /**
   * Default destructor
   */
  virtual ~MemorySlot() = default;

};

} // namespace L0

} // namespace HiCR
