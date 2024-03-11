/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file globalMemorySlot.hpp
 * @brief Provides a definition for the global memory slot class for the LPF backend
 * @author K. Dichev
 * @date 25/10/2023
 */
#pragma once

#include <hicr/L0/globalMemorySlot.hpp>
#include <hicr/L0/localMemorySlot.hpp>
#include <lpf/core.h>

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
class GlobalMemorySlot final : public HiCR::L0::GlobalMemorySlot
{
  public:

  /**
   * Constructor for a MemorySlot class for the LPF backend
   * @param[in] rank  Rank
   * @param[in] lpfMemSlot LPF slot this HiCR slot is associated with
   * @param[in] lpfSwapSlot LPF swap slot for global acquire/release calls this HiCR slot is associated with
   * @param[in] globalTag The global tag associated to this global memory slot (for exchange purposes)
   * @param[in] globalKey The global key associated to this global memory slot (for exchange purposes
   * @param[in] sourceLocalMemorySlot The local memory slot (if applicable) from whence this global memory slot is created
   */
  GlobalMemorySlot(size_t                                        rank,
                   lpf_memslot_t                                 lpfMemSlot,
                   lpf_memslot_t                                 lpfSwapSlot,
                   const HiCR::L0::GlobalMemorySlot::tag_t       globalTag             = 0,
                   const HiCR::L0::GlobalMemorySlot::globalKey_t globalKey             = 0,
                   std::shared_ptr<HiCR::L0::LocalMemorySlot>    sourceLocalMemorySlot = nullptr)
    : HiCR::L0::GlobalMemorySlot(globalTag, globalKey, sourceLocalMemorySlot),
      _rank(rank),
      _lpfMemSlot(lpfMemSlot),
      _lpfSwapSlot(lpfSwapSlot)
  {}

  /**
   * Default destructor
   */
  ~GlobalMemorySlot() = default;

  /**
   * Returns the rank to which this memory slot belongs
   *
   * \return The rank to which this memory slot belongs
   */
  const size_t getRank() { return _rank; }

  /**
   * Get LPF slot associated with this HiCR slot
   * @return LPF slot
   */
  lpf_memslot_t getLPFSlot() const { return _lpfMemSlot; }

  /**
   * Get LPF swap slot associated with this HiCR slot. This slot is only used for
   * acquire/release operations on the HiCR slot.
   * @return LPF slot
   */
  lpf_memslot_t getLPFSwapSlot() const { return _lpfSwapSlot; }

  private:

  /**
   * Remembers the MPI rank this memory slot belongs to
   */
  int _rank;

  /**
   * Internal LPF slot represented by this HiCR memory slot
   */
  const lpf_memslot_t _lpfMemSlot;

  /**
   * Internal LPF slot only used for global acquire/release
   * operations. It relies on IB Verbs atomic compare-and-swap
   */
  const lpf_memslot_t _lpfSwapSlot;
};

} // namespace L0

} // namespace lpf

} // namespace backend

} // namespace HiCR
