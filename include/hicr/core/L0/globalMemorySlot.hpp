/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file globalMemorySlot.hpp
 * @brief Provides a definition for a HiCR Global Memory Slot class
 * @author S. M. Martin
 * @date 4/10/2023
 */
#pragma once

#include <hicr/core/definitions.hpp>
#include <hicr/core/L0/localMemorySlot.hpp>
#include <utility>

namespace HiCR::L0
{

/**
 * This class represents an abstract definition for a Global Memory Slot resource in HiCR that:
 *
 * - Represents a contiguous segment of memory located in a non-local memory space
 */
class GlobalMemorySlot
{
  public:

  /**
   * Type definition for a global key (for exchanging global memory slots)
   */
  using globalKey_t = uint64_t;

  /**
   * Type definition for a communication tag
   */
  using tag_t = uint64_t;

  /**
   * Default constructor for a MemorySlot class
   *
   * \param[in] globalTag For global memory slots, indicates the subset of global memory slots this belongs to
   * \param[in] globalKey Unique identifier for that memory slot that this slot occupies.
   * \param[in] sourceLocalMemorySlot Indicates the source local memory slot (if any) that was promoted into this global memory slot.
   *                                  If none exists, a nullptr value is provided that encodes that the global memory slot is non-local (remote)
   */
  GlobalMemorySlot(const tag_t globalTag = 0, const globalKey_t globalKey = 0, std::shared_ptr<HiCR::L0::LocalMemorySlot> sourceLocalMemorySlot = nullptr)
    : _globalTag(globalTag),
      _globalKey(globalKey),
      _sourceLocalMemorySlot(std::move(sourceLocalMemorySlot))
  {}

  /**
   * Default destructor
   */
  virtual ~GlobalMemorySlot() = default;

  /**
   * Getter function for the memory slot's global tag
   * \returns The memory slot's global tag
   */
  [[nodiscard]] __INLINE__ tag_t getGlobalTag() const noexcept { return _globalTag; }

  /**
   * Getter function for the memory slot's global key
   * \returns The memory slot's global key
   */
  [[nodiscard]] __INLINE__ globalKey_t getGlobalKey() const noexcept { return _globalKey; }

  /**
   * Function to return the source local memory slot from which this global slot was created, if one exists (if not, it's a remote memory slot)
   *
   * \return A pointer to the local memory slot from which this global memory slot was created, if one exists. A null pointer, otherwise.
   */
  __INLINE__ std::shared_ptr<HiCR::L0::LocalMemorySlot> getSourceLocalMemorySlot() noexcept { return _sourceLocalMemorySlot; }

  /**
   * Function to serialize the global memory slot
   *
   * \return A pointer to the serialized representation of the global memory slot
   */
  virtual uint8_t *serialize() const
  {
    HICR_THROW_LOGIC("Trying to serialize a global memory slot; this is not supported in this backend\n");
    return nullptr;
  }

  private:

  /**
   * Only for global slots - identifies to which global memory slot subset this one belongs to
   */
  const tag_t _globalTag;

  /**
   * Only for global slots - Key identifier, unique positioning within the global memory slot subset
   */
  const globalKey_t _globalKey;

  /**
   * Pointer to the associated local memory slot (if one exists)
   */
  std::shared_ptr<HiCR::L0::LocalMemorySlot> const _sourceLocalMemorySlot = nullptr;
};

} // namespace HiCR::L0
