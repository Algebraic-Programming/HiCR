/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file sharedMemory.hpp
 * @brief This is a minimal memory manager for the sequential backend
 * @author S. M. Martin
 * @date 14/8/2023
 */

#pragma once

#include <hicr/backends/memoryManager.hpp>

namespace HiCR
{

namespace backend
{

namespace sequential
{

/**
 * Implementation of the SharedMemory/HWloc-based HiCR Shared Memory Backend.
 */
class MemoryManager final : public HiCR::backend::MemoryManager
{
  public:

 MemoryManager() : HiCR::backend::MemoryManager() {}
  ~MemoryManager() = default;


  private:

  /**
   * This stores the total system memory to check that allocations do not exceed it
   */
  size_t _totalSystemMem = 0;

  /**
    * This function returns the available allocatable size in the current system RAM
    *
    * @param[in] memorySpace Always zero, represents the system's RAM
    * @return The allocatable size within the system
    */
   __USED__ inline size_t getMemorySpaceSizeImpl(const memorySpaceId_t memorySpace) const override
   {
     return _totalSystemMem;
   }

   /**
    * This function returns the system physical memory size, which is what matters for a sequential program
    *
    * This is adapted from https://stackoverflow.com/a/2513561
    */
   __USED__ inline static size_t getTotalSystemMemory()
   {
     size_t pages = sysconf(_SC_PHYS_PAGES);
     size_t page_size = sysconf(_SC_PAGE_SIZE);
     return pages * page_size;
   }

   /**
    * Sequential backend implementation that returns a single memory space representing the entire RAM host memory.
    */
   __USED__ inline memorySpaceList_t queryMemorySpacesImpl() override
   {
     // Getting total system memory
     _totalSystemMem = sequential::MemoryManager::getTotalSystemMemory();

     // Only a single memory space is created
     return memorySpaceList_t({0});
   }

   /**
    * Queries the backend to update the internal state of the memory slot.
    * One main use case of this function is to update the number of messages received and sent to/from this slot.
    * This is a non-blocking, non-collective function.
    *
    * \param[in] memorySlot Memory slot to query for updates.
    */
   __USED__ inline void queryMemorySlotUpdatesImpl(const MemorySlot *memorySlot) override
   {
   }

   /**
    * Allocates memory in the current memory space (whole system)
    *
    * \param[in] memorySpace Memory space in which to perform the allocation.
    * \param[in] size Size of the memory slot to create
    * \returns The pointer of the newly allocated memory slot
    */
   __USED__ inline void *allocateLocalMemorySlotImpl(const memorySpaceId_t memorySpace, const size_t size) override
   {
     // Atempting to allocate the new memory slot
     auto ptr = malloc(size);

     // Check whether it was successful
     if (ptr == NULL) HICR_THROW_RUNTIME("Could not allocate memory of size %lu", size);

     // Now returning pointer
     return ptr;
   }

   /**
    * Associates a pointer locally-allocated manually and creates a local memory slot with it
    * \param[in] memorySlot The new local memory slot to register
    */
   __USED__ inline void registerLocalMemorySlotImpl(const MemorySlot *memorySlot) override
   {
     // Nothing to do here for this backend
   }

   /**
    * De-registers a memory slot previously registered
    *
    * \param[in] memorySlot Memory slot to deregister.
    */
   __USED__ inline void deregisterLocalMemorySlotImpl(MemorySlot *memorySlot) override
   {
     // Nothing to do here for this backend
   }

   __USED__ inline void deregisterGlobalMemorySlotImpl(MemorySlot *memorySlot) override
   {
     // Nothing to do here
   }

   /**
    * Exchanges memory slots among different local instances of HiCR to enable global (remote) communication
    *
    * \param[in] tag Identifies a particular subset of global memory slots
    * \param[in] memorySlots Array of local memory slots to make globally accessible
    */
   __USED__ inline void exchangeGlobalMemorySlots(const tag_t tag, const std::vector<globalKeyMemorySlotPair_t> &memorySlots) override
   {
     // Simply adding local memory slots to the global map
     for (const auto &entry : memorySlots)
     {
       const auto key = entry.first;
       const auto memorySlot = entry.second;
       registerGlobalMemorySlot(tag, key, memorySlot->getPointer(), memorySlot->getSize());
     }
   }

   /**
    * Backend-internal implementation of the freeLocalMemorySlot function
    *
    * \param[in] memorySlot Local memory slot to free up. It becomes unusable after freeing.
    */
   __USED__ inline void freeLocalMemorySlotImpl(MemorySlot *memorySlot) override
   {
     if (memorySlot->getPointer() == NULL) HICR_THROW_RUNTIME("Invalid memory slot(s) (%lu) provided. It either does not exit or represents a NULL pointer.", memorySlot->getId());

     free(memorySlot->getPointer());
   }

   /**
    * Backend-internal implementation of the isMemorySlotValid function
    *
    * \param[in] memorySlot Memory slot to check
    * \return True, if the referenced memory slot exists and is valid; false, otherwise
    */
   __USED__ bool isMemorySlotValidImpl(const MemorySlot *memorySlot) const override
   {
     // If it is NULL, it means it was never created
     if (memorySlot->getPointer() == NULL) return false;

     // Otherwise it is ok
     return true;
   }
};

} // namespace sequential
} // namespace backend
} // namespace HiCR
