/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file ascend.hpp
 * @brief This is a minimal backend for Ascend execution support
 * @author S. M. Martin & L. Terraciano
 * @date 06/10/2023
 */

#pragma once

#include <hicr/backend.hpp>
#include <hicr/common/definitions.hpp>

namespace HiCR
{

namespace backend
{

namespace ascend
{

/**
 * Implementation of the HiCR Ascend backend
 *
 */
class Ascend final : public Backend
{
  public:

  /**
   * Constructor for the ascend backend.
   */
  Ascend() : Backend() {}
  ~Ascend() = default;

  private:

  /**
   * This function returns the available allocatable size in the current system RAM
   *
   * @param[in] memorySpace Always zero, represents the system's RAM
   * @return The allocatable size within the system
   */
  __USED__ inline size_t getMemorySpaceSizeImpl(const memorySpaceId_t memorySpace) const override
  {
    HICR_THROW_LOGIC("This function has not been implemented yet");
  }

  /**
   * Ascend backend implementation that returns a single compute element.
   */
  __USED__ inline computeResourceList_t queryComputeResourcesImpl() override
  {
    HICR_THROW_LOGIC("This function has not been implemented yet");
  }

  /**
   * Ascend backend implementation that returns a single memory space representing the entire RAM host memory.
   */
  __USED__ inline memorySpaceList_t queryMemorySpacesImpl() override
  {
    HICR_THROW_LOGIC("This function has not been implemented yet");
  }

  __USED__ inline std::unique_ptr<ProcessingUnit> createProcessingUnitImpl(computeResourceId_t resource) const override
  {
    HICR_THROW_LOGIC("This function has not been implemented yet");
  }

  __USED__ inline void memcpyImpl(MemorySlot *destination, const size_t dst_offset, MemorySlot *source, const size_t src_offset, const size_t size) override
  {
    HICR_THROW_LOGIC("This function has not been implemented yet");
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
    HICR_THROW_LOGIC("This function has not been implemented yet");
  }

  /**
   * Implementation of the fence operation for the ascend backend. In this case, nothing needs to be done, as
   * the memcpy operation is synchronous. This means that it's mere execution (whether immediate or deferred)
   * ensures its completion.
   */
  __USED__ inline void fenceImpl(const tag_t tag, const globalKeyToMemorySlotArrayMap_t &globalSlots) override
  {
    HICR_THROW_LOGIC("This function has not been implemented yet");
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
    HICR_THROW_LOGIC("This function has not been implemented yet");
  }

  /**
   * Associates a pointer locally-allocated manually and creates a local memory slot with it
   * \param[in] memorySlot The new local memory slot to register
   */
  __USED__ inline void registerLocalMemorySlotImpl(const MemorySlot *memorySlot) override
  {
    HICR_THROW_LOGIC("This function has not been implemented yet");
  }

  /**
   * De-registers a memory slot previously registered
   *
   * \param[in] memorySlot Memory slot to deregister.
   */
  __USED__ inline void deregisterLocalMemorySlotImpl(MemorySlot *memorySlot) override
  {
    HICR_THROW_LOGIC("This function has not been implemented yet");
  }

  __USED__ inline void deregisterGlobalMemorySlotImpl(MemorySlot *memorySlot) override
  {
    HICR_THROW_LOGIC("This function has not been implemented yet");
  }

  /**
   * Exchanges memory slots among different local instances of HiCR to enable global (remote) communication
   *
   * \param[in] tag Identifies a particular subset of global memory slots
   * \param[in] memorySlots Array of local memory slots to make globally accessible
   */
  __USED__ inline void exchangeGlobalMemorySlots(const tag_t tag, const std::vector<globalKeyMemorySlotPair_t> &memorySlots) override
  {
    HICR_THROW_LOGIC("This function has not been implemented yet");
  }

  /**
   * Backend-internal implementation of the freeLocalMemorySlot function
   *
   * \param[in] memorySlot Local memory slot to free up. It becomes unusable after freeing.
   */
  __USED__ inline void freeLocalMemorySlotImpl(MemorySlot *memorySlot) override
  {
    HICR_THROW_LOGIC("This function has not been implemented yet");
  }

  /**
   * Backend-internal implementation of the isMemorySlotValid function
   *
   * \param[in] memorySlot Memory slot to check
   * \return True, if the referenced memory slot exists and is valid; false, otherwise
   */
  __USED__ bool isMemorySlotValidImpl(const MemorySlot *memorySlot) const override
  {
    HICR_THROW_LOGIC("This function has not been implemented yet");
  }
};

} // namespace ascend
} // namespace backend
} // namespace HiCR
