/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file memorySpace.hpp
 * @brief This file implements the memory space class for the shared memory backend
 * @author S. M. Martin
 * @date 12/12/2023
 */

#pragma once

#include <hicr/definitions.hpp>
#include <hicr/L0/memorySpace.hpp>

namespace HiCR
{

namespace backend
{

namespace sharedMemory
{

namespace L0
{

/**
 * This class represents a NUMA domain, i.e., addressable memory in a multi-core system with non-uniform access times
 */
class MemorySpace : public HiCR::L0::MemorySpace
{
  public:

  /**
   * Constructor for the memory space class of the shared memory backend
   *
   * \param size The maximum allocatable size detected for this memory space
   * \param hwlocObject HWLoc object for associated to this memory space
   * \param bindingSupport The HWLoc binding type supported by this memory space
   */
  MemorySpace(const size_t size) : HiCR::L0::MemorySpace(size) {}

  /**
   * Default destructor
   */
  ~MemorySpace() = default;

  __USED__ inline std::string getType() const override { return "NUMA Domain"; }

};

} // namespace L0

} // namespace sharedMemory

} // namespace backend

} // namespace HiCR
