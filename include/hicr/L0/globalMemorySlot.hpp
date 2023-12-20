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

#include <hicr/common/definitions.hpp>
#include <hicr/L0/localMemorySlot.hpp>

namespace HiCR
{

namespace L0
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
  typedef uint64_t globalKey_t;

  /**
   * Type definition for a communication tag
   */
  typedef uint64_t tag_t;

  /**
   * Default constructor for a MemorySlot class
   *
   * \param[in] globalTag For global memory slots, indicates the subset of global memory slots this belongs to
   * \param[in] globalKey Unique identifier for that memory slot that this slot occupies.
   * \param[in] sourceLocalMemorySlot Indicates the source local memory slot (if any) that was promoted into this global memory slot.
   *                                  If none exists, a nullptr value is provided that encodes that the global memory slot is non-local (remote)
   */
  GlobalMemorySlot(
    const tag_t globalTag = 0,
    const globalKey_t globalKey = 0,
    HiCR::L0::LocalMemorySlot* sourceLocalMemorySlot = nullptr) :
     _globalTag(globalTag),
     _globalKey(globalKey),
     _sourceLocalMemorySlot(sourceLocalMemorySlot)
  {
  }

  /**
   * Default destructor
   */
  virtual ~GlobalMemorySlot() = default;

  /**
   * Getter function for the memory slot's global tag
   * \returns The memory slot's global tag
   */
  __USED__ inline tag_t getGlobalTag() const noexcept { return _globalTag; }

  /**
   * Getter function for the memory slot's global key
   * \returns The memory slot's global key
   */
  __USED__ inline globalKey_t getGlobalKey() const noexcept { return _globalKey; }

  /**
   * Getter function for the memory slot's received message counter
   * \returns The memory slot's received message counter
   */
  __USED__ inline size_t getMessagesRecv() const noexcept { return _messagesRecv; }

  /**
   * Getter function for the memory slot's sent message counter
   * \returns The memory slot's sent message counter
   */
  __USED__ inline size_t getMessagesSent() const noexcept { return _messagesSent; }

  /**
   * Increase counter function for the memory slot's received message counter
   */
  __USED__ inline void increaseMessagesRecv() noexcept { _messagesRecv = _messagesRecv + 1; }

  /**
   * Increase counter function for the memory slot's sent message counter
   */
  __USED__ inline void increaseMessagesSent() noexcept { _messagesSent = _messagesSent + 1; }

  /**
   * Gets the pointer for the received message counter
   * \returns The pointer to the received message counter
   */
  __USED__ inline __volatile__ size_t *getMessagesRecvPointer() noexcept { return &_messagesRecv; }

  /**
   * Gets the pointer for the sent message counter
   * \returns The pointer to the sent message counter
   */
  __USED__ inline __volatile__ size_t *getMessagesSentPointer() noexcept { return &_messagesSent; }

  /**
  * Function to return the source local memory slot from which this global slot was created, if one exists (if not, it's a remote memory slot)
  */
  __USED__ HiCR::L0::LocalMemorySlot* getSourceLocalMemorySlot() noexcept { return _sourceLocalMemorySlot; }

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
  HiCR::L0::LocalMemorySlot* const _sourceLocalMemorySlot = 0;

  /**
   * Messages received into this slot
   */
  __volatile__ size_t _messagesRecv = 0;

  /**
   * Messages sent from this slot
   */
  __volatile__ size_t _messagesSent = 0;
};

} // namespace L0

} // namespace HiCR
