/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file memorySlot.hpp
 * @brief Provides a n abstractdefinition for all HiCR Memory Slot classes
 * @author S. M. Martin
 * @date 10/1/2024
 */
#pragma once

#include <cstddef>
#include <hicr/definitions.hpp>

namespace HiCR
{

namespace L0
{

/**
 * This class represents an abstract definition for a Memory Slot resource in HiCR that:
 *
 * - Represents a contiguous segment of memory
 * - Contains counters for received and sent messages
 */
class MemorySlot
{
  public:

  /**
   * Default destructor
   */
  virtual ~MemorySlot() = default;

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
   * Setter function for the memory slot's received message counter
   * @param[in] count The memory slot's recv message counter to set
   */
  __USED__ inline void setMessagesRecv(const size_t count) noexcept { _messagesRecv = count; }

  /**
   * Setter function for the memory slot's sent message counter
   * @param[in] count The memory slot's sent message counter to set
   */
  __USED__ inline void setMessagesSent(const size_t count) noexcept { _messagesSent = count; }

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

  protected:

  /**
   * Protected constructor for a MemorySlot class
   */
  MemorySlot() {}

  private:

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
