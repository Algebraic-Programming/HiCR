/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file memorySlot.hpp
 * @brief Provides a definition for a HiCR ProcessingUnit class
 * @author S. M. Martin
 * @date 4/10/2023
 */
#pragma once

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <hicr/common/definitions.hpp>

namespace HiCR
{

/**
 * Type definition for a generic memory slot identifier
 */
typedef boost::uuids::uuid memorySlotId_t;

/**
 * Type definition for a global key (for exchanging global memory slots)
 */
typedef uint64_t globalKey_t;

/**
 * Type definition for a communication tag
 */
typedef uint64_t tag_t;

/**
 * This class represents an abstract definition for a Memory Slot resource in HiCR that:
 *
 * - Represents a contiguous segment within a memory space, with a starting address and a size
 */
class MemorySlot
{
  public:

  /**
   * Getter function for the memory slot's id
   * \returns The memory slot's unique id
   */
  __USED__ inline memorySlotId_t getId() const noexcept { return _id; }

  /**
   * Getter function for the memory slot's pointer
   * \returns The memory slot's internal pointer
   */
  __USED__ inline void *getPointer() const noexcept { return _pointer; }

  /**
   * Getter function for the memory slot's size
   * \returns The memory slot's size
   */
  __USED__ inline size_t getSize() const noexcept { return _size; }

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
  __USED__ inline void increaseMessagesRecv() noexcept { _messagesRecv++; }

  /**
   * Increase counter function for the memory slot's sent message counter
   */
  __USED__ inline void increaseMessagesSent() noexcept { _messagesSent++; }

  /**
   * Gets the pointer for the received message counter
   * \returns The pointer to the received message counter
   */
  __USED__ inline size_t *getMessagesRecvPointer() noexcept { return &_messagesRecv; }

  /**
   * Gets the pointer for the sent message counter
   * \returns The pointer to the sent message counter
   */
  __USED__ inline size_t *getMessagesSentPointer() noexcept { return &_messagesSent; }

    /**
   * Default constructor for a MemorySlot class
   *
   * \param[in] pointer The pointer corresponding to an address in a given memory space
   * \param[in] size The size (in bytes) of the memory slot, assumed to be contiguous
   * \param[in] creationType Indicates the way in which the memory slot was created (e.g., registered manually or allocated)
   * \param[in] localityType Indicates whether this is a local or global memory slot
   * \param[in] globalTag For global memory slots, indicates the subset of global memory slots this belongs to
   * \param[in] globalKey Unique identifier for that memory slot that this slot occupies.
   */

  MemorySlot(
    void *const pointer,
    const size_t size,
    const tag_t globalTag = 0,
    const globalKey_t globalKey = 0) : _id(boost::uuids::random_generator()()), _pointer(pointer), _size(size), _globalTag(globalTag), _globalKey(globalKey)
  {
  }

  ~MemorySlot() = default;


  private:

  /**
   * Unique identifier for the given memory slot
   */
  const memorySlotId_t _id;

  /**
   * Pointer to the local memory address containing this slot
   */
  void *const _pointer;

  /**
   * Size of the memory slot
   */
  const size_t _size;

  /**
   * Only for global slots - identifies to which global memory slot subset this one belongs to
   */
  const tag_t _globalTag;

  /**
   * Only for global slots - Key identifier, unique positioning within the global memory slot subset
   */
  const globalKey_t _globalKey;

  /**
   * Messages received into this slot
   */
  size_t _messagesRecv = 0;

  /**
   * Messages sent from this slot
   */
  size_t _messagesSent = 0;
};

} // namespace HiCR
