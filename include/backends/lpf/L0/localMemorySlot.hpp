/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file localMemorySlot.hpp
 * @brief Provides a definition for the local memory slot class for the LPF backend
 * @author K. Dichev
 * @date 25/10/2023
 */
#pragma once

#include <hicr/L0/localMemorySlot.hpp>
#include <hicr/L0/memorySpace.hpp>
#include <lpf/core.h>

/**
 * #CHECK(f...) Checks if an LPF function returns LPF_SUCCESS, else
 * it prints an error message
 */
#define CHECK(f...) if (f != LPF_SUCCESS) { HICR_THROW_RUNTIME("LPF Backend Error: '%s'", #f); }

namespace HiCR
{

namespace backend
{

namespace lpf
{

namespace L0
{

/**
 * This class is the definition for a Memory Slot resource for the LPF backend
 */
class LocalMemorySlot final : public HiCR::L0::LocalMemorySlot
{
  public:

  /**
   * Constructor for a MemorySlot class for the LPF backend
   * @param[in] rank  Rank
   * @param[in] lpfMemSlot LPF slot this HiCR slot is associated with
   * @param[in] pointer Pointer to the memory address associated with this HiCR slot
   * @param[in] size Comm size
   */
  LocalMemorySlot(
    lpf_memslot_t lpfMemSlot,
    void *const pointer,
    const size_t size,
    HiCR::L0::MemorySpace* memorySpace) : HiCR::L0::LocalMemorySlot(pointer, size, memorySpace), _lpfMemSlot(lpfMemSlot)
  {
  }

  /**
   * Default destructor
   */
  ~LocalMemorySlot() = default;

  /**
   * Get LPF slot associated with this HiCR slot
   * @return LPF slot
   */
  lpf_memslot_t getLPFSlot() const { return _lpfMemSlot; }

  private:

  /**
   * Internal LPF slot represented by this HiCR memory slot
   */
  const lpf_memslot_t _lpfMemSlot;
};

} // namespace L0

} // namespace lpf

} // namespace backend

} // namespace HiCR
