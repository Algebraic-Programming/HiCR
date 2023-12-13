/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file memorySpace.hpp
 * @brief Provides a base definition for a HiCR MemorySpace class
 * @author S. M. Martin
 * @date 13/12/2023
 */
#pragma once

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
  virtual const size_t getSize() const { return _size; }

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
  MemorySpace(const size_t size) : _size(size) {};

  /**
   * The memory space size, defined at construction time
   */
  const size_t _size;
};

} // namespace L0

} // namespace HiCR
