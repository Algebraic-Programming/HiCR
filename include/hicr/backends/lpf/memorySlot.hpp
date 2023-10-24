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
   * Constructor for a MemorySlot class for the LPF backend
   
   */
  MemorySlot(
    size_t rank,
    lpf_memslot_t lpfMemSlot,
    void *const pointer,
    const size_t size,
    const tag_t globalTag = 0,
    const globalKey_t globalKey = 0) : HiCR::MemorySlot(pointer, size, globalTag, globalKey), _rank(rank), _lpfMemSlot(lpfMemSlot)
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

  lpf_memslot_t getLPFSlot() const {return _lpfMemSlot;}

  bool operator< (const MemorySlot &slot) const {
      if (this->getGlobalTag() < slot.getGlobalTag())
          return true;
      else if (this->getGlobalTag() > slot.getGlobalTag())
          return false;
      else {
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

} // namespace lpf

} // namespace backend

} // namespace HiCR
