/*
 *   Copyright 2025 Huawei Technologies Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file localMemorySlot.hpp
 * @brief Provides a definition for a HiCR Local Memory Slot class
 * @author S. M. Martin
 * @date 4/10/2023
 */
#pragma once

#include <memory>
#include <hicr/core/definitions.hpp>
#include <hicr/core/memorySpace.hpp>
#include <utility>

namespace HiCR
{
class CommunicationManager;
}

namespace HiCR
{

/**
 * This class represents an abstract definition for a Local Memory Slot resource in HiCR that:
 *
 * - Represents a contiguous segment within a memory space in the local system, with a starting address and a size
 */
class LocalMemorySlot
{
  friend class HiCR::CommunicationManager;

  public:

  /**
   * Default constructor for a LocalMemorySlot class
   *
   * \param[in] pointer The pointer corresponding to an address in a given memory space
   * \param[in] size The size (in bytes) of the memory slot, assumed to be contiguous
   * \param[in] memorySpace Pointer to the memory space that this memory slot belongs to. NULL, if the memory slot is global (remote)
   */
  LocalMemorySlot(void *const pointer, const size_t size, std::shared_ptr<HiCR::MemorySpace> memorySpace = nullptr)
    : _pointer(pointer),
      _size(size),
      _memorySpace(std::move(memorySpace))
  {
    _messagesRecv = &_messagesRecvStorage;
    _messagesSent = &_messagesSentStorage;
  }

  /**
   * Default destructor
   */
  virtual ~LocalMemorySlot() = default;

  /**
   * Getter function for the memory slot's pointer
   * \returns The memory slot's internal pointer
   */
  __INLINE__ void *&getPointer() noexcept { return _pointer; }

  /**
   * Getter function for the memory slot's size
   * \returns The memory slot's size
   */
  [[nodiscard]] __INLINE__ size_t getSize() const noexcept { return _size; }

  /**
   * Getter function for the memory slot's associated memory space
   * \returns The memory slot's associated memory space
   */
  [[nodiscard]] __INLINE__ std::shared_ptr<HiCR::MemorySpace> getMemorySpace() const noexcept { return _memorySpace; }

  /**
   * Getter function for the memory slot's received message counter
   * \returns The memory slot's received message counter
   */
  [[nodiscard]] __INLINE__ size_t getMessagesRecv() const noexcept { return *_messagesRecv; }

  /**
   * Getter function for the memory slot's sent message counter
   * \returns The memory slot's sent message counter
   */
  [[nodiscard]] __INLINE__ size_t getMessagesSent() const noexcept { return *_messagesSent; }

  private:

  /**
   * Increase counter function for the memory slot's received message counter
   */
  __INLINE__ void increaseMessagesRecv() noexcept { *_messagesRecv = *_messagesRecv + 1; }

  /**
   * Increase counter function for the memory slot's sent message counter
   */
  __INLINE__ void increaseMessagesSent() noexcept { *_messagesSent = *_messagesSent + 1; }

  /**
   * Gets the pointer for the received message counter
   * \returns The pointer to the received message counter
   */
  __INLINE__ __volatile__ size_t *&getMessagesRecvPointer() noexcept { return _messagesRecv; }

  /**
   * Gets the pointer for the sent message counter
   * \returns The pointer to the sent message counter
   */
  __INLINE__ __volatile__ size_t *&getMessagesSentPointer() noexcept { return _messagesSent; }

  /**
   * Setter function for the memory slot's received message counter
   * @param[in] count The memory slot's recv message counter to set
   */
  __INLINE__ void setMessagesRecv(const size_t count) noexcept { *_messagesRecv = count; }

  /**
   * Setter function for the memory slot's sent message counter
   * @param[in] count The memory slot's sent message counter to set
   */
  __INLINE__ void setMessagesSent(const size_t count) noexcept { *_messagesSent = count; }

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
  std::shared_ptr<MemorySpace> const _memorySpace;

  /**
   * Messages received into this slot
   */
  __volatile__ size_t *_messagesRecv;

  /**
   * Messages sent from this slot
   */
  __volatile__ size_t *_messagesSent;

  /**
   * Internal storage for messages received
   */
  __volatile__ size_t _messagesRecvStorage{0};

  /**
   * Internal storage for messages sent
   */
  __volatile__ size_t _messagesSentStorage{0};
};

} // namespace HiCR
