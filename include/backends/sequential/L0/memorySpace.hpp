/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file memorySpace.hpp
 * @brief This file implements the memory space class for the sequential backend
 * @author S. M. Martin
 * @date 12/12/2023
 */

#pragma once

#include <hicr/L0/memorySpace.hpp>
#include <hicr/common/definitions.hpp>

namespace HiCR
{

namespace backend
{

namespace sequential
{

namespace L0
{

/** 
 * This class represents a memory space, as visible by the sequential backend. That is, the entire RAM that the running CPU has access to.
 */
class MemorySpace final : public HiCR::L0::MemorySpace
{
  public:

  /**
   * This function returns the system physical memory size, which is what matters for a sequential program
   *
   * This is adapted from https://stackoverflow.com/a/2513561
   *
   * \return the system physical memory size
   */
  __USED__ inline static size_t getTotalSystemMemory()
  {
    size_t pages = sysconf(_SC_PHYS_PAGES);
    size_t page_size = sysconf(_SC_PAGE_SIZE);
    return pages * page_size;
  }

  /**
   * Constructor for the compute resource class of the sequential backend
   */
  MemorySpace() : HiCR::L0::MemorySpace(getTotalSystemMemory()) {};

  /**
   * Default destructor
   */
  ~MemorySpace() = default;

  __USED__ inline std::string getType() const override { return "Host RAM"; }
};

} // namespace L0

} // namespace sequential

} // namespace backend

} // namespace HiCR
