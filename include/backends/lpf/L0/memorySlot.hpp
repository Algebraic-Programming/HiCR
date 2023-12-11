/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file memorySlot.hpp
 * @brief Provides a definition for the memory slot class for the LPF backend
 * @author K. Dichev
 * @date 25/10/2023
 */
#pragma once

#include <hicr/L0/memorySlot.hpp>
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
class MemorySlot final : public HiCR::L0::MemorySlot
{
  public:

  /**
   * Constructor for a MemorySlot class for the LPF backend
   * @param[in] rank  Rank
   * @param[in] lpfMemSlot LPF slot this HiCR slot is associated with
   * @param[in] pointer Pointer to the memory address associated with this HiCR slot
   * @param[in] size Comm size
   * @param[in] globalTag
   * @param[in] globalKey
   */
  MemorySlot(
    size_t rank,
    lpf_memslot_t lpfMemSlot,
    void *const pointer,
    const size_t size,
    const HiCR::L0::MemorySlot::tag_t globalTag = 0,
    const HiCR::L0::MemorySlot::globalKey_t globalKey = 0) : HiCR::L0::MemorySlot(pointer, size, globalTag, globalKey), _rank(rank), _lpfMemSlot(lpfMemSlot)
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
  const size_t getRank() { return _rank; }

  /**
   * Get LPF slot associated with this HiCR slot
   * @return LPF slot
   */
  lpf_memslot_t getLPFSlot() const { return _lpfMemSlot; }

  /**
   * @param[in] slot Right-hand side slot in comparison
   * @return true if left-hand side is smaller according to (tag,key) comparison
   * The comparison operator is provided for the hash table
   * MemoryManager::initMsgCnt
   */
  bool operator<(const MemorySlot &slot) const
  {
    if (this->getGlobalTag() < slot.getGlobalTag())
      return true;
    else if (this->getGlobalTag() > slot.getGlobalTag())
      return false;
    else
    {
      return (this->getGlobalKey() < slot.getGlobalKey());
    }
  }

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

} // namespace L0

} // namespace lpf

} // namespace backend

} // namespace HiCR
