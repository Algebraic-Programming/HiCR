/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file memorySpace.hpp
 * @brief Provides a base definition for a HiCR MemorySpace class
 * @author S. M. Martin & O. Korakitis
 * @date 13/12/2023
 */
#pragma once

#include <hicr/exceptions.hpp>
#include <string>

namespace HiCR
{

namespace L0
{

/**
 * This class represents an abstract definition for a Memory Space that:
 *
 * - Represents a autonomous unit of byte-addressable memory (e.g., host memory, NUMA domain, device global RAM)
 * - The space is assumed to be contiguous and have a fixed sized determined at construction time
 * - This is a copy-able class that only contains metadata
 *
 * A Device object may comprise one or more such Memory Spaces
 * on which data can be allocated, copied and communicated among
 * different Memory Spaces, provided there is connectivity
 */
class MemorySpace
{
  public:

  /**
   * Indicates what type of memory space is contained in this instance
   *
   * \return A string containing a human-readable description of the memory space type
   */
  virtual std::string getType() const = 0;

  /**
   * Returns the memory space's size
   *
   * \return The memory space's size
   */
  __USED__ virtual inline const size_t getSize() const { return _size; }

  /**
   *  If supported, obtain the amount of memory currently in use.
   * In conjunction with the total size above, the user may deduce
   * information like, usage%, if a particular allocation will be
   * possible etc.
   *
   * \return The current memory usage for this memory space
   */
  __USED__ virtual inline size_t getUsage() const { return _usage; };

  /**
   * Registers an increase in the used memory size of the current memory space, either by allocation or manual registering
   *
   * \param delta How much (in bytes) has the memory usage increased
   */
  __USED__ inline void increaseUsage(const size_t delta)
  {
    if (_usage + delta > _size) HICR_THROW_LOGIC("Increasing memory space usage beyond its capacity (current_usage + increase > capacity | %lu + %lu > %lu)\n", _usage, delta, _size);

    _usage += delta;
  }

  /**
   * Registers a decrease in the used memory size of the current memory space, either by freeing or manual deregistering
   *
   * \param delta How much (in bytes) has the memory usage decreased
   */
  __USED__ inline void decreaseUsage(const size_t delta)
  {
    if (delta > _usage) HICR_THROW_LOGIC("Decreasing memory space usage below zero (probably a bug in HiCR) (current_usage - decrease < 0 | %lu - %lu < 0)\n", _usage, delta);

    _usage -= delta;
  }

  /**
   * Default destructor
   */
  virtual ~MemorySpace() = default;

  protected:

  /**
   * Constructor for the MemorySpace class
   *
   * \param[in] size The size of the memory space to create
   */
  MemorySpace(const size_t size) : _size(size){};

  /**
   * The memory space size, defined at construction time
   */
  const size_t _size;

  /**
   * This variable keeps track of the memory space usage (through allocation and frees)
   */
  size_t _usage = 0;
};

} // namespace L0

} // namespace HiCR
