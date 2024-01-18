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
#include <hicr/L0/memorySlot.hpp>
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
class LocalMemorySlot : public MemorySlot
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
    void *const pointer,
    const size_t size,
    std::shared_ptr<HiCR::L0::MemorySpace> memorySpace = NULL) : MemorySlot(),
                                                                 _pointer(pointer),
                                                                 _size(size),
                                                                 _memorySpace(memorySpace)
  {
  }

  /**
   * Default destructor
   */
  virtual ~LocalMemorySlot() = default;

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
   * Getter function for the memory slot's associated memory space
   * \returns The memory slot's associated memory space
   */
  __USED__ inline std::shared_ptr<HiCR::L0::MemorySpace> getMemorySpace() const noexcept { return _memorySpace; }

  /**
   * Setter function for the memory slot's sent message counter
   * @param[in] count The memory slot's sent message counter to set
   */
  __USED__ inline void setMessagesSent(const size_t count) noexcept { _messagesSent = count; }
  /**
   * Setter function for the memory slot's received message counter
   * @param[in] count The memory slot's recv message counter to set
   */
  __USED__ inline void setMessagesRecv(const size_t count) noexcept { _messagesRecv = count; }

  /**
   * Getter function for the memory slot's sent message counter
   * \returns The memory slot's sent message counter
   */
  __USED__ inline size_t getMessagesSent() const noexcept { return _messagesSent; }

  /**
   * Getter function for the memory slot's received message counter
   * \returns The memory slot's received message counter
   */
  __USED__ inline size_t getMessagesRecv() const noexcept { return _messagesRecv; }

  private:

  /**
   * Messages received into this slot
   */
  __volatile__ size_t _messagesRecv = 0;

  /**
   * Messages sent from this slot
   */
  __volatile__ size_t _messagesSent = 0;

  /**
   * Pointer to the local memory address containing this slot
   */
  void *const _pointer;

  /**
   * Size of the memory slot
   */
  const size_t _size;

  /**
   * Memory space that this memory slot belongs to
   */
  std::shared_ptr<L0::MemorySpace> const _memorySpace;
};

} // namespace L0

} // namespace HiCR
