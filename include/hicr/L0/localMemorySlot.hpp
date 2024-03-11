/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file localMemorySlot.hpp
 * @brief Provides a definition for a HiCR Local Memory Slot class
 * @author S. M. Martin
 * @date 4/10/2023
 */
#pragma once

#include <memory>
#include <hicr/definitions.hpp>
#include <hicr/L0/memorySpace.hpp>

namespace HiCR
{

namespace L0
{

/**
 * This class represents an abstract definition for a Local Memory Slot resource in HiCR that:
 *
 * - Represents a contiguous segment within a memory space in the local system, with a starting address and a size
 */
class LocalMemorySlot
{
  public:

  /**
   * Default constructor for a LocalMemorySlot class
   *
   * \param[in] pointer The pointer corresponding to an address in a given memory space
   * \param[in] size The size (in bytes) of the memory slot, assumed to be contiguous
   * \param[in] memorySpace Pointer to the memory space that this memory slot belongs to. NULL, if the memory slot is global (remote)
   */
  LocalMemorySlot(
    void *const                            pointer,
    const size_t                           size,
    std::shared_ptr<HiCR::L0::MemorySpace> memorySpace = NULL) : _pointer(pointer),
                                                                 _size(size),
                                                                 _memorySpace(memorySpace)
  {
    _messagesRecvStorage = 0;
    _messagesSentStorage = 0;
    _messagesRecv        = &_messagesRecvStorage;
    _messagesSent        = &_messagesSentStorage;
  }

  /**
   * Default destructor
   */
  virtual ~LocalMemorySlot() = default;

  /**
   * Getter function for the memory slot's pointer
   * \returns The memory slot's internal pointer
   */
  __USED__ inline void *&getPointer() noexcept { return _pointer; }

  /**
   * Getter function for the memory slot's size
   * \returns The memory slot's size
   */
  __USED__ inline size_t getSize() const noexcept { return _size; }

  /**
   * Getter function for the memory slot's associated memory space
   * \returns The memory slot's associated memory space
   */
  __USED__ inline std::shared_ptr<HiCR::L0::MemorySpace> getMemorySpace() const noexcept { return _memorySpace; }

  /**
   * Getter function for the memory slot's received message counter
   * \returns The memory slot's received message counter
   */
  __USED__ inline size_t getMessagesRecv() const noexcept { return *_messagesRecv; }

  /**
   * Getter function for the memory slot's sent message counter
   * \returns The memory slot's sent message counter
   */
  __USED__ inline size_t getMessagesSent() const noexcept { return *_messagesSent; }

  /**
   * Setter function for the memory slot's received message counter
   * @param[in] count The memory slot's recv message counter to set
   */
  __USED__ inline void setMessagesRecv(const size_t count) noexcept { *_messagesRecv = count; }

  /**
   * Setter function for the memory slot's sent message counter
   * @param[in] count The memory slot's sent message counter to set
   */
  __USED__ inline void setMessagesSent(const size_t count) noexcept { *_messagesSent = count; }

  /**
   * Increase counter function for the memory slot's received message counter
   */
  __USED__ inline void increaseMessagesRecv() noexcept { *_messagesRecv = *_messagesRecv + 1; }

  /**
   * Increase counter function for the memory slot's sent message counter
   */
  __USED__ inline void increaseMessagesSent() noexcept { *_messagesSent = *_messagesSent + 1; }

  /**
   * Gets the pointer for the received message counter
   * \returns The pointer to the received message counter
   */
  __USED__ inline __volatile__ size_t *&getMessagesRecvPointer() noexcept { return _messagesRecv; }

  /**
   * Gets the pointer for the sent message counter
   * \returns The pointer to the sent message counter
   */
  __USED__ inline __volatile__ size_t *&getMessagesSentPointer() noexcept { return _messagesSent; }

  private:

  /**
   * Pointer to the local memory address containing this slot
   */
  void *_pointer;

  /**
   * Size of the memory slot
   */
  const size_t _size;

  /**
   * Memory space that this memory slot belongs to
   */
  std::shared_ptr<L0::MemorySpace> const _memorySpace;

  /**
   * Messages received into this slot
   */
  __volatile__ size_t *_messagesRecv;

  /**
   * Messages sent from this slot
   */
  __volatile__ size_t *_messagesSent;

  __volatile__ size_t _messagesRecvStorage;
  __volatile__ size_t _messagesSentStorage;
};

} // namespace L0

} // namespace HiCR
