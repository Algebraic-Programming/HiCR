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

#include <hicr/common/definitions.hpp>

namespace HiCR
{

/**
 * Type definition for a generic memory slot identifier
 */
typedef uint64_t memorySlotId_t;

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
  * Enumeration to determine how the memory slot has been created
  */
 enum creationType_t
 {
   /**
    * When a memory slot is allocated, it uses the backend's own allocator. Its internal pointer may not be changed, and requires the use of the backend free operation
    */
   allocated,

   /**
    * When a memory slot is manually registered, it is assigned to the backend having been allocated elsewhere or is global
    */
   registered
 };

 /**
  * Enumeration to determine whether the memory slot is local or global
  */
 enum localityType_t
 {
   /**
    * When a memory slot is local, it was created by the current backend and can be freed and accessed directly
    */
   local,

   /**
    * When a memory slot is global, it was exchanged with other backends (possibly remote)
    */
   global
 };

 MemorySlot(
   memorySlotId_t id,
   void* const pointer,
   const size_t size,
   const creationType_t creationType,
   const localityType_t localityType,
   const tag_t globalTag = 0,
   const globalKey_t globalKey = 0
   ) : _id(id), _pointer(pointer), _size(size), _creationType(creationType), _localityType(localityType), _globalTag(globalTag), _globalKey(globalKey)
 {

 }

 ~MemorySlot() = default;


 __USED__ inline memorySlotId_t getId() const noexcept { return _id; }

 __USED__ inline void* getPointer() const noexcept { return _pointer; }

 __USED__ inline size_t getSize() const noexcept { return _size; }

 __USED__ inline creationType_t getCreationType() const noexcept { return _creationType; }

 __USED__ inline localityType_t getLocalityType() const noexcept { return _localityType; }

 __USED__ inline tag_t getGlobalTag() const noexcept { return _globalTag; }

 __USED__ inline globalKey_t getGlobalKey() const noexcept { return _globalKey; }

 __USED__ inline size_t getMessagesRecv() const noexcept { return _messagesRecv; }

 __USED__ inline size_t getMessagesSent() const noexcept { return _messagesSent; }

 __USED__ inline void increaseMessagesRecv() noexcept { _messagesRecv++; }

  __USED__ inline void increaseMessagesSent() noexcept  { _messagesSent++; }

  __USED__ inline size_t* getMessagesRecvPointer() noexcept { return &_messagesRecv; }

  __USED__ inline size_t* getMessagesSentPointer() noexcept { return &_messagesSent; }

  private:

 /**
  * Unique identifier for the given memory slot
  */
 const memorySlotId_t _id;


 /**
  * Pointer to the local memory address containing this slot
  */
 void * const _pointer;

 /**
  * Size of the memory slot
  */
 const size_t _size;

 /**
  * Stores how the memory slot was created
  */
 const creationType_t _creationType;

 /**
  * Stores the locality of the memory slot
  */
 const localityType_t _localityType;

 /**
  * Only for global slots - Tag identifier
  */
 const tag_t _globalTag;

 /**
  * Only for global slots - Key identifier
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
