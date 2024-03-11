/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file localMemorySlot.hpp
 * @brief Provides a definition for the local memory slot class for the MPI backend
 * @author S. M. Martin
 * @date 19/10/2023
 */
#pragma once

#include <hicr/L0/localMemorySlot.hpp>

namespace HiCR
{

namespace backend
{

namespace mpi
{

namespace L0
{

/**
 * This class represents an abstract definition for a Memory Slot resource in HiCR that:
 *
 * - Represents a contiguous segment within a memory space, with a starting address and a size
 */
class LocalMemorySlot final : public HiCR::L0::LocalMemorySlot
{
  public:

  /**
   * Constructor for a MemorySlot class for the MPI backend
   *
   * \param[in] pointer If this is a local slot (same rank as this the running process), this pointer indicates the address of the local memory segment
   * \param[in] size The size (in bytes) of the memory slot, assumed to be contiguous
   * \param[in] memorySpace The memory space from whence this memory slot was created
   */
  LocalMemorySlot(
    void *const                            pointer,
    const size_t                           size,
    std::shared_ptr<HiCR::L0::MemorySpace> memorySpace)
    : HiCR::L0::LocalMemorySlot(pointer, size, memorySpace)
  {
  }

  ~LocalMemorySlot() = default;
};

} // namespace L0

} // namespace mpi

} // namespace backend

} // namespace HiCR
