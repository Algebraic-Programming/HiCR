/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file memoryManager.hpp
 * @brief This file implements the memory manager class for the MPI backend
 * @author S. M. Martin
 * @date 11/9/2023
 */

#pragma once

#include <hicr/backends/memoryManager.hpp>
#include <hicr/backends/mpi/memorySlot.hpp>
#include <hicr/common/definitions.hpp>
#include <mpi.h>

namespace HiCR
{

namespace backend
{

namespace mpi
{

/**
 * Implementation of the HiCR MPI backend
 *
 * This backend is very useful for testing other HiCR modules in isolation (unit tests) without involving the use of threading, which might incur side-effects
 */
class MemoryManager final : public HiCR::backend::MemoryManager
{
  public:

  /**
   * Constructor for the mpi backend.
   *
   * \param[in] comm The MPI subcommunicator to use in the communication operations in this backend.
   * If not specified, it will use MPI_COMM_WORLD
   */
  MemoryManager(MPI_Comm comm = MPI_COMM_WORLD) : HiCR::backend::MemoryManager(), _comm(comm)
  {
    MPI_Comm_size(_comm, &_size);
    MPI_Comm_rank(_comm, &_rank);
  }

  ~MemoryManager() = default;

  private:

  /**
   * Default MPI communicator to use for this backend
   */
  const MPI_Comm _comm;

  /**
   * Number of MPI processes in the communicator
   */
  int _size;

  /**
   * MPI rank corresponding to this process
   */
  int _rank;

  /**
   * This function returns the available allocatable size in the current system RAM
   *
   * @param[in] memorySpace Always zero, represents the system's RAM
   * @return The allocatable size within the system
   */
  __USED__ inline size_t getMemorySpaceSizeImpl(const memorySpaceId_t memorySpace) const override
  {
    HICR_THROW_RUNTIME("This backend provides no support for memory spaces");
  }

  /**
   * The MPI backend offers no memory spaces
   */
  __USED__ inline memorySpaceList_t queryMemorySpacesImpl() override
  {
    // No memory spaces are provided by this backend
    return memorySpaceList_t({});
  }

  __USED__ inline void lockMPIWindow(const int rank, MPI_Win *window)
  {
    // Locking MPI window to ensure the messages arrives before returning
    auto status = MPI_Win_lock(MPI_LOCK_EXCLUSIVE, rank, 0, *window);

    // Checking correct locking
    if (status != MPI_SUCCESS) HICR_THROW_RUNTIME("Failed to run lock MPI data window for rank %d, MPI Window pointer 0x%lX", rank, (uint64_t)window);
  }

  __USED__ inline void unlockMPIWindow(const int rank, MPI_Win *window)
  {
    // Unlocking window after copy is completed
    auto status = MPI_Win_unlock(rank, *window);

    // Checking correct unlocking
    if (status != MPI_SUCCESS) HICR_THROW_RUNTIME("Failed to run unlock MPI data window for rank %d, MPI Window pointer 0x%lX", rank, (uint64_t)window);
  }

  __USED__ inline void increaseWindowCounter(const int rank, MPI_Win *window)
  {
    // This operation should be possible to do in one go with MPI_Accumulate or MPI_Fetch_and_op. However, the current implementation of openMPI deadlocks
    // on these operations, so I rather do the whole thing manually.

    // Locking MPI window to ensure the messages arrives before returning
    lockMPIWindow(rank, window);

    // Getting remote counter into the local conter
    size_t accumulatorBuffer = 0;
    auto status = MPI_Get(&accumulatorBuffer, 1, MPI_UNSIGNED_LONG, rank, 0, 1, MPI_UNSIGNED_LONG, *window);
    if (status != MPI_SUCCESS) HICR_THROW_RUNTIME("Failed to increase remote message counter (on operation: MPI_Get) for rank %d, MPI Window pointer 0x%lX", rank, (uint64_t)window);

    // Waiting for the get operation to finish
    MPI_Win_flush(rank, *window);

    // Adding one to the local counter
    accumulatorBuffer++;

    // Replacing the remote counter with the local counter
    status = MPI_Put(&accumulatorBuffer, 1, MPI_UNSIGNED_LONG, rank, 0, 1, MPI_UNSIGNED_LONG, *window);

    // Checking execution status
    if (status != MPI_SUCCESS) HICR_THROW_RUNTIME("Failed to increase remote message counter (on operation: MPI_Put) for rank %d, MPI Window pointer 0x%lX", rank, (uint64_t)window);

    // Unlocking window after copy is completed
    unlockMPIWindow(rank, window);
  }

  __USED__ inline void memcpyImpl(HiCR::MemorySlot *destinationSlotPtr, const size_t dst_offset, HiCR::MemorySlot *sourceSlotPtr, const size_t sourceOffset, const size_t size) override
  {
    // Getting up-casted pointer for the execution unit
    auto destination = dynamic_cast<MemorySlot *>(destinationSlotPtr);

    // Checking whether the execution unit passed is compatible with this backend
    if (destination == NULL) HICR_THROW_LOGIC("The passed destination memory slot is not supported by this backend\n");

    // Getting up-casted pointer for the execution unit
    auto source = dynamic_cast<MemorySlot *>(sourceSlotPtr);

    // Checking whether the execution unit passed is compatible with this backend
    if (source == NULL) HICR_THROW_LOGIC("The passed source memory slot is not supported by this backend\n");

    // Getting ranks for the involved processes
    const auto sourceRank = source->getRank();
    const auto destinationRank = destination->getRank();

    // Checking whether source and/or remote are remote
    bool isSourceRemote = sourceRank != _rank;
    bool isDestinationRemote = destinationRank != _rank;

    // Sanity checks
    if (isSourceRemote == true && isDestinationRemote == true) HICR_THROW_LOGIC("Trying to use MPI backend perform a remote to remote copy between slots");

    // Calculating pointers
    [[maybe_unused]] auto destinationPointer = (void *)(((uint8_t *)destination->getPointer()) + dst_offset);
    [[maybe_unused]] auto sourcePointer = (void *)(((uint8_t *)source->getPointer()) + sourceOffset);

    // Getting data windows for the involved processes (if necessary)
    [[maybe_unused]] auto sourceDataWindow = source->getDataWindow();
    [[maybe_unused]] auto destinationDataWindow = destination->getDataWindow();

    // Getting recv message count windows for the involved processes (if necessary)
    [[maybe_unused]] auto sourceSentMessageWindow = source->getSentMessageCountWindow();
    [[maybe_unused]] auto destinationRecvMessageWindow = destination->getRecvMessageCountWindow();

    // Perform a get if the source is remote and destination is local
    if (isSourceRemote == true && isDestinationRemote == false)
    {
      // Locking MPI window to ensure the messages arrives before returning
      lockMPIWindow(sourceRank, sourceDataWindow);

      // Executing the get operation
      auto status = MPI_Get(destinationPointer, size, MPI_BYTE, sourceRank, sourceOffset, size, MPI_BYTE, *sourceDataWindow);

      // Checking execution status
      if (status != MPI_SUCCESS) HICR_THROW_RUNTIME("Failed to run MPI_Get");

      // Unlocking window after copy is completed
      unlockMPIWindow(sourceRank, sourceDataWindow);

      // Increasing the remote sent message counter and local destination received message counter
      increaseWindowCounter(sourceRank, sourceSentMessageWindow);
      destination->increaseMessagesRecv();
    }

    // Perform a put if source is local and destination is remote
    if (isSourceRemote == false && isDestinationRemote == true)
    {
      // Locking MPI window to ensure the messages arrives before returning
      lockMPIWindow(destinationRank, destinationDataWindow);

      // Executing the put operation
      auto status = MPI_Put(sourcePointer, size, MPI_BYTE, destinationRank, dst_offset, size, MPI_BYTE, *destinationDataWindow);

      if (status != MPI_SUCCESS) HICR_THROW_RUNTIME("Failed to run data MPI_Put");

      // Unlocking window after copy is completed
      unlockMPIWindow(destinationRank, destinationDataWindow);

      // Increasing the remote received message counter and local sent message counter
      increaseWindowCounter(destinationRank, destinationRecvMessageWindow);
      source->increaseMessagesSent();
    }

    // If both elements are local, then use memcpy directly
    if (isSourceRemote == false && isDestinationRemote == false)
    {
      // Performing actual mem copy
      ::memcpy(destinationPointer, sourcePointer, size);

      // Increasing message send/received counters
      destination->increaseMessagesRecv();
      source->increaseMessagesSent();
    }
  }

  /**
   * Queries the backend to update the internal state of the memory slot.
   * One main use case of this function is to update the number of messages received and sent to/from this slot.
   * This is a collective function
   *
   * \param[in] memorySlot Memory slot to query for updates.
   */
  __USED__ inline void queryMemorySlotUpdatesImpl(const HiCR::MemorySlot *memorySlot) override
  {
  }

  /**
   * Implementation of the fence operation for the mpi backend. For every single window corresponding
   * to a memory slot associated with the tag, a fence needs to be executed
   */
  __USED__ inline void fenceImpl(const tag_t tag) override
  {
    // For every key-valued subset, and its elements, execute a fence
    for (const auto &entry : _globalMemorySlotTagKeyMap[tag])
    {
      // Getting memory slot id
      auto memorySlotPtr = entry.second;

      // Getting up-casted pointer for the execution unit
      auto memorySlot = dynamic_cast<MemorySlot *>(memorySlotPtr);

      // Checking whether the execution unit passed is compatible with this backend
      if (memorySlot == NULL) HICR_THROW_LOGIC("The memory slot is not supported by this backend\n");

      // Attempting fence
      auto status = MPI_Win_fence(0, *memorySlot->getDataWindow());

      // Check for possible errors
      if (status != MPI_SUCCESS) HICR_THROW_RUNTIME("Failed to fence on MPI window on fence operation for tag %lu.", tag);

      // Attempting fence
      status = MPI_Win_fence(0, *memorySlot->getRecvMessageCountWindow());

      // Check for possible errors
      if (status != MPI_SUCCESS) HICR_THROW_RUNTIME("Failed to fence on MPI window on fence operation for tag %lu.", tag);

      // Attempting fence
      status = MPI_Win_fence(0, *memorySlot->getSentMessageCountWindow());

      // Check for possible errors
      if (status != MPI_SUCCESS) HICR_THROW_RUNTIME("Failed to fence on MPI window on fence operation for tag %lu.", tag);
    }
  }

  /**
   * Allocates memory in the current memory space (whole system)
   *
   * \param[in] memorySpace Memory space in which to perform the allocation.
   * \param[in] size Size of the memory slot to create
   * \returns The address of the newly allocated memory slot
   */
  __USED__ inline HiCR::MemorySlot *allocateLocalMemorySlotImpl(const memorySpaceId_t memorySpace, const size_t size) override
  {
    HICR_THROW_RUNTIME("This backend provides no support for memory allocation");
  }

  /**
   * Associates a pointer locally-allocated manually and creates a local memory slot with it
   *
   * \param[in] ptr Pointer to the local memory space
   * \param[in] size Size of the memory slot to register
   * \return A newly created memory slot
   */
  __USED__ inline MemorySlot *registerLocalMemorySlotImpl(void *const ptr, const size_t size) override
  {
    // Creating new memory slot object
    auto memorySlot = new MemorySlot(_rank, ptr, size);

    // Returning new memory slot pointer
    return memorySlot;
  }

  /**
   * De-registers a memory slot previously registered
   *
   * \param[in] memorySlot Pointer to the memory slot to deregister.
   */
  __USED__ inline void deregisterLocalMemorySlotImpl(HiCR::MemorySlot *memorySlot) override
  {
    // Nothing to do here for this backend
  }

  __USED__ inline void deregisterGlobalMemorySlotImpl(HiCR::MemorySlot *memorySlotPtr) override
  {
    // Getting up-casted pointer for the execution unit
    auto memorySlot = dynamic_cast<MemorySlot *>(memorySlotPtr);

    // Checking whether the execution unit passed is compatible with this backend
    if (memorySlot == NULL) HICR_THROW_LOGIC("The memory slot is not supported by this backend\n");

    auto status = MPI_Win_free(memorySlot->getDataWindow());
    if (status != MPI_SUCCESS) HICR_THROW_RUNTIME("On deregister global memory slot, could not free MPI data window");

    status = MPI_Win_free(memorySlot->getRecvMessageCountWindow());
    if (status != MPI_SUCCESS) HICR_THROW_RUNTIME("On deregister global memory slot, could not free MPI recv message count window");

    status = MPI_Win_free(memorySlot->getSentMessageCountWindow());
    if (status != MPI_SUCCESS) HICR_THROW_RUNTIME("On deregister global memory slot, could not free MPI sent message count window");
  }

  /**
   * Exchanges memory slots among different local instances of HiCR to enable global (remote) communication
   *
   * \param[in] tag Identifies a particular subset of global memory slots
   * \param[in] memorySlots Array of local memory slots to make globally accessible
   */
  __USED__ inline void exchangeGlobalMemorySlotsImpl(const tag_t tag, const std::vector<globalKeyMemorySlotPair_t> &memorySlots) override
  {
    // Obtaining local slots to exchange
    int localSlotCount = (int)memorySlots.size();

    // Obtaining the local slots to exchange per process in the communicator
    std::vector<int> perProcessSlotCount(_size);
    MPI_Allgather(&localSlotCount, 1, MPI_INT, perProcessSlotCount.data(), 1, MPI_INT, _comm);

    // Calculating respective offsets
    std::vector<int> perProcessSlotOffsets(_size);
    int currentOffset = 0;
    for (int i = 0; i < _size; i++)
    {
      perProcessSlotOffsets[i] += currentOffset;
      currentOffset += perProcessSlotCount[i];
    }

    // Calculating number of global slots
    int globalSlotCount = 0;
    for (const auto count : perProcessSlotCount) globalSlotCount += count;

    // Allocating storage for local and global memory slot sizes, keys and process id
    std::vector<size_t> localSlotSizes(localSlotCount);
    std::vector<size_t> globalSlotSizes(globalSlotCount);
    std::vector<globalKey_t> localSlotKeys(localSlotCount);
    std::vector<globalKey_t> globalSlotKeys(globalSlotCount);
    std::vector<int> localSlotProcessId(localSlotCount);
    std::vector<int> globalSlotProcessId(globalSlotCount);

    // Filling in the local size and keys storage
    for (size_t i = 0; i < memorySlots.size(); i++)
    {
      const auto key = memorySlots[i].first;
      const auto memorySlot = memorySlots[i].second;
      localSlotSizes[i] = memorySlot->getSize();
      localSlotKeys[i] = key;
      localSlotProcessId[i] = _rank;
    }

    // Exchanging global sizes, keys and process ids
    MPI_Allgatherv(localSlotSizes.data(), localSlotCount, MPI_UNSIGNED_LONG, globalSlotSizes.data(), perProcessSlotCount.data(), perProcessSlotOffsets.data(), MPI_UNSIGNED_LONG, _comm);
    MPI_Allgatherv(localSlotKeys.data(), localSlotCount, MPI_UNSIGNED_LONG, globalSlotKeys.data(), perProcessSlotCount.data(), perProcessSlotOffsets.data(), MPI_UNSIGNED_LONG, _comm);
    MPI_Allgatherv(localSlotProcessId.data(), localSlotCount, MPI_INT, globalSlotProcessId.data(), perProcessSlotCount.data(), perProcessSlotOffsets.data(), MPI_INT, _comm);

    // Now also creating pointer vector to remember local pointers, when required for memcpys
    std::vector<void *> globalSlotPointers(globalSlotCount);
    size_t localPointerPos = 0;
    for (size_t i = 0; i < globalSlotPointers.size(); i++)
    {
      // If the rank associated with this slot is remote, don't store the pointer, otherwise store it.
      if (globalSlotProcessId[i] != _rank)
        globalSlotPointers[i] = NULL;
      else
      {
        const auto memorySlot = memorySlots[localPointerPos++].second;
        globalSlotPointers[i] = memorySlot->getPointer();
      }
    }

    // Now creating global slots and their MPI windows
    for (size_t i = 0; i < globalSlotProcessId.size(); i++)
    {
      // Creating new memory slot object
      auto memorySlot = new MemorySlot(
        globalSlotProcessId[i],
        globalSlotPointers[i],
        globalSlotSizes[i],
        tag,
        globalSlotKeys[i]);

      // Allocating MPI windows
      memorySlot->getDataWindow() = new MPI_Win;
      memorySlot->getRecvMessageCountWindow() = new MPI_Win;
      memorySlot->getSentMessageCountWindow() = new MPI_Win;

      // Debug info
      // printf("Rank: %u, Pos %lu, GlobalSlot %lu, Key: %lu, Size: %lu, LocalPtr: 0x%lX, %s\n", _rank, i, globalSlotId, globalSlotKeys[i], globalSlotSizes[i], (uint64_t)globalSlotPointers[i], globalSlotProcessId[i] == _rank ? "x" : "");

      // Creating MPI window for data transferring
      auto status = MPI_Win_create(
        globalSlotPointers[i],
        globalSlotProcessId[i] == _rank ? globalSlotSizes[i] : 0,
        1,
        MPI_INFO_NULL,
        _comm,
        memorySlot->getDataWindow());

      if (status != MPI_SUCCESS) HICR_THROW_RUNTIME("Failed to create MPI data window on exchange global memory slots.");

      // Creating MPI window for message received count transferring
      status = MPI_Win_create(
        globalSlotProcessId[i] == _rank ? memorySlot->getMessagesRecvPointer() : NULL,
        globalSlotProcessId[i] == _rank ? sizeof(size_t) : 0,
        1,
        MPI_INFO_NULL,
        _comm,
        memorySlot->getRecvMessageCountWindow());

      if (status != MPI_SUCCESS) HICR_THROW_RUNTIME("Failed to create MPI received message count window on exchange global memory slots.");

      // Creating MPI window for message sent count transferring
      status = MPI_Win_create(
        globalSlotProcessId[i] == _rank ? memorySlot->getMessagesSentPointer() : NULL,
        globalSlotProcessId[i] == _rank ? sizeof(size_t) : 0,
        1,
        MPI_INFO_NULL,
        _comm,
        memorySlot->getSentMessageCountWindow());

      if (status != MPI_SUCCESS) HICR_THROW_RUNTIME("Failed to create MPI sent message count window on exchange global memory slots.");

      // Registering global slot
      registerGlobalMemorySlot(memorySlot);
    }
  }

  /**
   * Frees up a local memory slot reserved from this memory space
   *
   * \param[in] memorySlot Local memory slot to free up. It becomes unusable after freeing.
   */
  __USED__ inline void freeLocalMemorySlotImpl(HiCR::MemorySlot *memorySlot) override
  {
    HICR_THROW_RUNTIME("This backend provides no support for memory freeing");
  }
};

} // namespace mpi
} // namespace backend
} // namespace HiCR
