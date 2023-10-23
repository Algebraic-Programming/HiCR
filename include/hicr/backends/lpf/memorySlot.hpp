/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file memorySlot.hpp
 * @brief Provides a definition for the memory slot class for the MPI backend
 * @author S. M. Martin
 * @date 19/10/2023
 */
#pragma once

#include <hicr/memorySlot.hpp>
#include <lpf/core.h>

namespace HiCR
{

namespace backend
{

namespace lpf
{

/**
 * This class is the definition for a Memory Slot resource for the LPF backend
 */
class MemorySlot final : public HiCR::MemorySlot
{
  public:

  /**
   * Constructor for a MemorySlot class for the MPI backend
   *
   * \param[in] rank Rank to which this memory slot belongs
   * \param[in] pointer If this is a local slot (same rank as this the running process), this pointer indicates the address of the local memory segment
   * \param[in] size The size (in bytes) of the memory slot, assumed to be contiguous
   * \param[in] globalTag For global memory slots, indicates the subset of global memory slots this belongs to
   * \param[in] globalKey Unique identifier for that memory slot that this slot occupies.
   */
  MemorySlot(
    int rank,
    lpf_memslot_t lpfMemSlot,
    lpf_ rank,
    void *const pointer,
    const size_t size,
    const tag_t globalTag = 0,
    const globalKey_t globalKey = 0) : HiCR::MemorySlot(pointer, size, globalTag, globalKey),
                                       _rank(rank), _lpfMemSlot(lpfMemSlot)
  {
  }

  /**
   * Default destructor
   */
  ~MemorySlot() = default;

  /**
   * Returns the rank to which this memory slot belongs
   *
   * \return The rank to which this memory slot belongs
   */
  const int getRank() { return _rank; }




  private:

  /**
   * Remembers the MPI rank this memory slot belongs to
   */
  int _rank;

  /**
   * Internal LPF slot represented by this HiCR memory slot
   */
  const lpf_memslot_t _lpfMemSlot;
};

} // namespace lpf

} // namespace backend

} // namespace HiCR
