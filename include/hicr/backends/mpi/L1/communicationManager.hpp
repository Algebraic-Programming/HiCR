/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file memoryManager.hpp
 * @brief This file implements the communication manager class for the MPI backend
 * @author S. M. Martin
 * @date 19/12/2023
 */

#pragma once

#include <mpi.h>
#include <set>
#include <hicr/core/definitions.hpp>
#include <hicr/core/L0/localMemorySlot.hpp>
#include <hicr/core/L1/communicationManager.hpp>
#include "../L0/localMemorySlot.hpp"
#include "../L0/globalMemorySlot.hpp"

namespace HiCR::backend::mpi::L1
{

/**
 * Implementation of the HiCR MPI backend
 *
 * This backend is very useful for testing other HiCR modules in isolation (unit tests) without involving the use of threading, which might incur side-effects
 */
class CommunicationManager final : public HiCR::L1::CommunicationManager
{
  public:

  /**
   * Constructor for the mpi backend.
   *
   * \param[in] comm The MPI subcommunicator to use in the communication operations in this backend.
   * If not specified, it will use MPI_COMM_WORLD
   */
  CommunicationManager(MPI_Comm comm = MPI_COMM_WORLD)
    : HiCR::L1::CommunicationManager(),
      _comm(comm)
  {
    MPI_Comm_size(_comm, &_size);
    MPI_Comm_rank(_comm, &_rank);
  }

  ~CommunicationManager() override = default;

  /**
   * MPI Communicator getter
   * \return The internal MPI communicator used during the instantiation of this class
   */
  [[nodiscard]] const MPI_Comm getComm() const { return _comm; }

  /**
   * MPI Communicator size getter
   * \return The size of the internal MPI communicator used during the instantiation of this class
   */
  [[nodiscard]] const int getSize() const { return _size; }

  /**
   * MPI Communicator rank getter
   * \return The rank within the internal MPI communicator used during the instantiation of this class that corresponds to this instance
   */
  [[nodiscard]] const int getRank() const { return _rank; }

  private:

  /**
   * Default MPI communicator to use for this backend
   */
  const MPI_Comm _comm;

  /**
   * Number of MPI processes in the communicator
   */
  int _size{};

  /**
   * MPI rank corresponding to this process
   */
  int _rank{};

  /**
   * Unfortunately in MPI, we need to be able to access the Window in order to (collectively) free it.
   * Therefore, we need to keep track of all the windows we created, even if they have been deregistered
   * by the user. If even one instance loses track of the window, it cannot be freed.
   */
  HiCR::L1::CommunicationManager::globalMemorySlotTagKeyMap_t _deregisteredGlobalMemorySlotsTagKeyMap{};

  __INLINE__ void lockMPIWindow(int rank, MPI_Win *window, int MPILockType, int MPIAssert)
  {
    // Locking MPI window to ensure the messages arrives before returning
    int mpiStatus = 0;
    do {
      lock();
      mpiStatus = MPI_Win_lock(MPILockType, rank, MPIAssert, *window) != MPI_SUCCESS;
      unlock();
    }
    while (mpiStatus != MPI_SUCCESS);
  }

  __INLINE__ void unlockMPIWindow(int rank, MPI_Win *window)
  {
    // Unlocking window after copy is completed
    int mpiStatus = 0;
    do {
      lock();
      mpiStatus = MPI_Win_unlock(rank, *window) != MPI_SUCCESS;
      unlock();
    }
    while (mpiStatus != MPI_SUCCESS);
  }

  __INLINE__ void increaseWindowCounter(int rank, MPI_Win *window)
  {
    // This operation should be possible to do in one go with MPI_Accumulate or MPI_Fetch_and_op. However, the current implementation of openMPI deadlocks
    // on these operations, so I rather do the whole thing manually.

    // Locking MPI window to ensure the messages arrives before returning
    lockMPIWindow(rank, window, MPI_LOCK_EXCLUSIVE, 0);

    // Use atomic MPI operation to increment counter
    const size_t one   = 1;
    size_t       value = 0;

    // There is no datatype in MPI for size_t (the counters), but
    // MPI_AINT is supposed to be large enough and portable
    lock();
    auto status = MPI_Fetch_and_op(&one, &value, MPI_AINT, rank, 0, MPI_SUM, *window);
    unlock();

    // Checking execution status
    if (status != MPI_SUCCESS) HICR_THROW_RUNTIME("Failed to increase remote message counter (on operation: MPI_Put) for rank %d, MPI Window pointer %p", rank, window);

    // Unlocking window after copy is completed
    unlockMPIWindow(rank, window);
  }

  __INLINE__ void memcpyImpl(const std::shared_ptr<HiCR::L0::LocalMemorySlot>  &destinationSlot,
                             size_t                                             dst_offset,
                             const std::shared_ptr<HiCR::L0::GlobalMemorySlot> &sourceSlotPtr,
                             size_t                                             sourceOffset,
                             size_t                                             size) override
  {
    // Getting up-casted pointer for the execution unit
    auto source = dynamic_pointer_cast<mpi::L0::GlobalMemorySlot>(sourceSlotPtr);

    // Checking whether the execution unit passed is compatible with this backend
    if (source == nullptr) HICR_THROW_LOGIC("The passed source memory slot is not supported by this backend\n");

    // Getting ranks for the involved processes
    const auto sourceRank = source->getRank();

    // Check if we already acquired a lock on the memory slots
    bool isSourceSlotLockAcquired = source->getLockAcquiredValue();

    // Calculating pointer
    auto destinationPointer = (void *)(static_cast<uint8_t *>(destinationSlot->getPointer()) + dst_offset);

    // Getting data window for the involved processes
    auto sourceDataWindow = source->getDataWindow().get();

    // Getting recv message count window for the involved processes
    auto sourceSentMessageWindow = source->getSentMessageCountWindow().get();

    // Locking MPI window to ensure the messages arrives before returning. This will not exclude other processes from accessing the data (MPI_LOCK_SHARED)
    if (isSourceSlotLockAcquired == false) lockMPIWindow(sourceRank, sourceDataWindow, MPI_LOCK_SHARED, MPI_MODE_NOCHECK);

    // Executing the get operation
    {
      lock();
      auto status = MPI_Get(destinationPointer, (int)size, MPI_BYTE, sourceRank, (int)sourceOffset, (int)size, MPI_BYTE, *sourceDataWindow);
      unlock();
      if (status != MPI_SUCCESS) HICR_THROW_RUNTIME("Failed to run MPI_Get");
    }

    // Making sure the operation finished
    {
      lock();
      auto status = MPI_Win_flush(sourceRank, *sourceDataWindow);
      unlock();
      if (status != MPI_SUCCESS) HICR_THROW_RUNTIME("Failed to run MPI_Win_flush");
    }

    // Unlocking window, if taken, after copy is completed
    if (isSourceSlotLockAcquired == false) unlockMPIWindow(sourceRank, sourceDataWindow);

    // Increasing the remote sent message counter and local destination received message counter
    increaseWindowCounter(sourceRank, sourceSentMessageWindow);
    destinationSlot->increaseMessagesRecv();
  }

  __INLINE__ void memcpyImpl(const std::shared_ptr<HiCR::L0::GlobalMemorySlot> &destinationSlotPtr,
                             size_t                                             dst_offset,
                             const std::shared_ptr<HiCR::L0::LocalMemorySlot>  &sourceSlot,
                             size_t                                             sourceOffset,
                             size_t                                             size) override
  {
    // Getting up-casted pointer for the execution unit
    auto destination = dynamic_pointer_cast<mpi::L0::GlobalMemorySlot>(destinationSlotPtr);

    // Checking whether the execution unit passed is compatible with this backend
    if (destination == nullptr) HICR_THROW_LOGIC("The passed destination memory slot is not supported by this backend\n");

    // Getting ranks for the involved processes
    const auto destinationRank = destination->getRank();

    // Check if we already acquired a lock on the memory slots
    bool isDestinationSlotLockAcquired = destination->getLockAcquiredValue();

    // Calculating pointers
    auto sourcePointer = (void *)(static_cast<uint8_t *>(sourceSlot->getPointer()) + sourceOffset);

    // Getting data window for the involved processes
    auto destinationDataWindow = destination->getDataWindow().get();

    // Getting recv message count windows for the involved process
    auto destinationRecvMessageWindow = destination->getRecvMessageCountWindow().get();

    // Locking MPI window to ensure the messages arrives before returning. This will not exclude other processes from accessing the data (MPI_LOCK_SHARED)
    if (isDestinationSlotLockAcquired == false) lockMPIWindow(destinationRank, destinationDataWindow, MPI_LOCK_SHARED, MPI_MODE_NOCHECK);

    // Executing the put operation
    {
      lock();
      auto status = MPI_Put(sourcePointer, (int)size, MPI_BYTE, destinationRank, (int)dst_offset, (int)size, MPI_BYTE, *destinationDataWindow);
      unlock();
      if (status != MPI_SUCCESS) HICR_THROW_RUNTIME("Failed to run data MPI_Put");
    }

    // Making sure the operation finished
    {
      lock();
      auto status = MPI_Win_flush(destinationRank, *destinationDataWindow);
      unlock();
      if (status != MPI_SUCCESS) HICR_THROW_RUNTIME("Failed to run data MPI_Win_flush");
    }

    // Unlocking window, if taken, after copy is completed
    if (isDestinationSlotLockAcquired == false) unlockMPIWindow(destinationRank, destinationDataWindow);

    // Increasing the remote received message counter and local sent message counter
    sourceSlot->increaseMessagesSent();
    increaseWindowCounter(destinationRank, destinationRecvMessageWindow);
  }

  /**
   * Queries the backend to update the internal state of the memory slot.
   * One main use case of this function is to update the number of messages received and sent to/from this slot.
   * This is a collective function
   *
   * \param[in] memorySlot Memory slot to query for updates.
   */
  __INLINE__ void queryMemorySlotUpdatesImpl(std::shared_ptr<HiCR::L0::LocalMemorySlot> memorySlot) override {}

  /**
   * MPI-specific operations associated with the de-registration of a global memory slot
   * This operation is non-collective.
   *
   * In MPI we can not afford to lose track of the MPI windows, as they need to be freed collectively.
   * Therefore, if a particular instance requests the destruction of a memory slot but other instances
   * have lost track of the window, by deregistering the slot, the window cannot be freed.
   *
   * This should not be a problem in e.g., LPF, as the slots are just IDs and can be exchanged in order to
   * be collectively freed.
   *
   * \param[in] memorySlot The memory slot to deregister
   */
  __INLINE__ void deregisterGlobalMemorySlotImpl(const std::shared_ptr<HiCR::L0::GlobalMemorySlot> &memorySlot) override
  {
    // Getting up-casted pointer for the slot
    auto slot = dynamic_pointer_cast<mpi::L0::GlobalMemorySlot>(memorySlot);

    // Checking whether the slot passed is compatible with this backend
    if (slot == nullptr) HICR_THROW_LOGIC("The memory slot is not supported by this backend\n");

    // Getting the slot information
    const auto tag = slot->getGlobalTag();
    const auto key = slot->getGlobalKey();

    // Storing the deregistered slot, and it is guaranteed that the (MPI) type is correct
    _deregisteredGlobalMemorySlotsTagKeyMap[tag][key] = slot;
  }

  /**
   * Implementation of the fence operation for the mpi backend.
   * A barrier is sufficient, as MPI_Win_lock/MPI_Win_unlock passive
   * synchronization is used to transfer data
   *
   * It is assumed that the base class has already locked the mutex before calling this function.
   */
  __INLINE__ void fenceImpl(HiCR::L0::GlobalMemorySlot::tag_t tag) override
  {
    MPI_Barrier(_comm);

    // Call the slot destruction collective routine
    destroyGlobalMemorySlotsCollectiveImpl(tag);
  }

  __INLINE__ void destroyGlobalMemorySlotsCollectiveImpl(HiCR::L0::GlobalMemorySlot::tag_t tag)
  {
    // Destruction of global memory slots marked for destruction
    // note: MPI expects int, not size_t as the parameter for allgather which we use here, so we have to work with int
    int              localDestroySlotsCount = (int)getGlobalMemorySlotsToDestroyPerTag()[tag].size();
    std::vector<int> perProcessDestroySlotCount(_size);

    // Obtaining the number of slots to destroy per process in the communicator
    MPI_Allgather(&localDestroySlotsCount, 1, MPI_INT, perProcessDestroySlotCount.data(), 1, MPI_INT, _comm);

    // Calculating respective offsets; TODO fix offset types for both this method and exchangeGlobalMemorySlotsImpl
    std::vector<int> perProcessSlotOffsets(_size);
    int              currentOffset = 0;
    for (int i = 0; i < _size; i++)
    {
      perProcessSlotOffsets[i] += currentOffset;
      currentOffset += perProcessDestroySlotCount[i];
    }

    // Calculating number of global slots to destroy
    int globalDestroySlotsCount = 0;
    for (const auto count : perProcessDestroySlotCount) globalDestroySlotsCount += count;

    // If there are no slots to destroy from any instance, return to avoid a second round of collectives
    if (globalDestroySlotsCount == 0) return;

    // Allocating storage for global memory slot keys
    std::vector<HiCR::L0::GlobalMemorySlot::globalKey_t> localDestroySlotKeys(localDestroySlotsCount);
    std::vector<HiCR::L0::GlobalMemorySlot::globalKey_t> globalDestroySlotKeys(globalDestroySlotsCount);

    // Filling in the local keys storage
    for (auto i = 0; i < localDestroySlotsCount; i++)
    {
      const auto memorySlot   = getGlobalMemorySlotsToDestroyPerTag()[tag][i];
      const auto key          = memorySlot->getGlobalKey();
      localDestroySlotKeys[i] = key;
    }

    // Exchanging global keys
    MPI_Allgatherv(localDestroySlotKeys.data(),
                   localDestroySlotsCount,
                   MPI_UNSIGNED_LONG,
                   globalDestroySlotKeys.data(),
                   perProcessDestroySlotCount.data(),
                   perProcessSlotOffsets.data(),
                   MPI_UNSIGNED_LONG,
                   _comm);

    // Deduplicating the global keys, as more than one process might want to destroy the same key
    std::set<HiCR::L0::GlobalMemorySlot::globalKey_t> globalDestroySlotKeysSet(globalDestroySlotKeys.begin(), globalDestroySlotKeys.end());

    // Now we can iterate over the global slots to destroy one by one
    for (auto key : globalDestroySlotKeysSet)
    {
      std::shared_ptr<HiCR::L0::GlobalMemorySlot> memorySlot = nullptr;
      // Getting the memory slot to destroy
      // First check the standard map
      if (getGlobalMemorySlotTagKeyMap()[tag].contains(key))
      {
        memorySlot = getGlobalMemorySlotTagKeyMap()[tag].at(key);
        // Deregister because a later destroy will try and fail to destroy
        getGlobalMemorySlotTagKeyMap()[tag].erase(key);
      }
      // If not found, check the deregistered map
      else if (_deregisteredGlobalMemorySlotsTagKeyMap[tag].contains(key))
      {
        memorySlot = _deregisteredGlobalMemorySlotsTagKeyMap[tag].at(key);
        _deregisteredGlobalMemorySlotsTagKeyMap[tag].erase(key);
      }
      else
        HICR_THROW_FATAL("Could not find memory slot to destroy in this backend. Tag: %d, Key: %lu", tag, key);

      // Destroying the memory slot collectively; there might be a case where the slot is not found, due to double calls to destroy
      destroyGlobalMemorySlotImpl(memorySlot);
    }
  }

  __INLINE__ void exchangeGlobalMemorySlotsImpl(HiCR::L0::GlobalMemorySlot::tag_t tag, const std::vector<globalKeyMemorySlotPair_t> &memorySlots) override
  {
    // Obtaining local slots to exchange
    int localSlotCount = (int)memorySlots.size();

    // Obtaining the local slots to exchange per process in the communicator
    std::vector<int> perProcessSlotCount(_size);
    lock();
    MPI_Allgather(&localSlotCount, 1, MPI_INT, perProcessSlotCount.data(), 1, MPI_INT, _comm);
    unlock();

    // Calculating respective offsets
    std::vector<int> perProcessSlotOffsets(_size);
    int              currentOffset = 0;
    for (int i = 0; i < _size; i++)
    {
      perProcessSlotOffsets[i] += currentOffset;
      currentOffset += perProcessSlotCount[i];
    }

    // Calculating number of global slots
    int globalSlotCount = 0;
    for (const auto count : perProcessSlotCount) globalSlotCount += count;

    // Allocating storage for local and global memory slot sizes, keys and process id
    std::vector<size_t>                                  localSlotSizes(localSlotCount);
    std::vector<size_t>                                  globalSlotSizes(globalSlotCount);
    std::vector<HiCR::L0::GlobalMemorySlot::globalKey_t> localSlotKeys(localSlotCount);
    std::vector<HiCR::L0::GlobalMemorySlot::globalKey_t> globalSlotKeys(globalSlotCount);
    std::vector<int>                                     localSlotProcessId(localSlotCount);
    std::vector<int>                                     globalSlotProcessId(globalSlotCount);

    // Filling in the local size and keys storage
    for (size_t i = 0; i < memorySlots.size(); i++)
    {
      const auto key        = memorySlots[i].first;
      const auto memorySlot = std::dynamic_pointer_cast<HiCR::backend::mpi::L0::LocalMemorySlot>(memorySlots[i].second);
      if (memorySlot.get() == nullptr) HICR_THROW_LOGIC("Trying to use MPI to promote a non-MPI local memory slot.");
      localSlotSizes[i]     = memorySlot->getSize();
      localSlotKeys[i]      = key;
      localSlotProcessId[i] = _rank;
    }

    // Exchanging global sizes, keys and process ids
    lock();
    MPI_Allgatherv(
      localSlotSizes.data(), localSlotCount, MPI_UNSIGNED_LONG, globalSlotSizes.data(), perProcessSlotCount.data(), perProcessSlotOffsets.data(), MPI_UNSIGNED_LONG, _comm);
    MPI_Allgatherv(
      localSlotKeys.data(), localSlotCount, MPI_UNSIGNED_LONG, globalSlotKeys.data(), perProcessSlotCount.data(), perProcessSlotOffsets.data(), MPI_UNSIGNED_LONG, _comm);
    MPI_Allgatherv(localSlotProcessId.data(), localSlotCount, MPI_INT, globalSlotProcessId.data(), perProcessSlotCount.data(), perProcessSlotOffsets.data(), MPI_INT, _comm);
    unlock();

    // Now also creating pointer vector to remember local pointers, when required for memcpys
    std::vector<void **>                                    globalSlotPointers(globalSlotCount);
    std::vector<std::shared_ptr<HiCR::L0::LocalMemorySlot>> globalSourceSlots(globalSlotCount);
    size_t                                                  localPointerPos = 0;
    for (size_t i = 0; i < globalSlotPointers.size(); i++)
    {
      // If the rank associated with this slot is remote, don't store the pointer, otherwise store it.
      if (globalSlotProcessId[i] != _rank)
      {
        globalSlotPointers[i] = nullptr;
        globalSourceSlots[i]  = nullptr;
      }
      else
      {
        const auto memorySlot = memorySlots[localPointerPos++].second;
        globalSlotPointers[i] = &memorySlot->getPointer();
        globalSourceSlots[i]  = memorySlot;
      }
    }

    // Now creating global slots and their MPI windows
    for (size_t i = 0; i < globalSlotProcessId.size(); i++)
    {
      // Creating new memory slot object
      auto memorySlot = std::make_shared<mpi::L0::GlobalMemorySlot>(globalSlotProcessId[i], tag, globalSlotKeys[i], globalSourceSlots[i]);

      // Allocating MPI windows
      memorySlot->getDataWindow()             = std::make_unique<MPI_Win>();
      memorySlot->getRecvMessageCountWindow() = std::make_unique<MPI_Win>();
      memorySlot->getSentMessageCountWindow() = std::make_unique<MPI_Win>();

      // Termporary storage for the pointer returned by MPI_Win_Allocate. We will assign this a new internal storage to the local memory slot
      void *ptr = nullptr;

      // Creating MPI window for data transferring
      lock();
      auto status = MPI_Win_allocate(globalSlotProcessId[i] == _rank ? (int)globalSlotSizes[i] : 0, 1, MPI_INFO_NULL, _comm, &ptr, memorySlot->getDataWindow().get());
      MPI_Win_set_errhandler(*memorySlot->getDataWindow(), MPI_ERRORS_RETURN);
      unlock();

      // Unfortunately, we need to do an effective duplucation of the original local memory slot storage
      // since no modern MPI library supports MPI_Win_create over user-allocated storage anymore
      if (globalSlotProcessId[i] == _rank)
      {
        // Copying existing data over to the new storage
        std::memcpy(ptr, *(globalSlotPointers[i]), globalSlotSizes[i]);

        // Freeing up memory
        lock();
        MPI_Free_mem(*(globalSlotPointers[i]));
        unlock();

        // Swapping pointers
        *(globalSlotPointers[i]) = ptr;
      }

      if (status != MPI_SUCCESS) HICR_THROW_RUNTIME("Failed to create MPI data window on exchange global memory slots.");

      // Creating MPI window for message received count transferring
      lock();
      status = MPI_Win_allocate(globalSlotProcessId[i] == _rank ? sizeof(size_t) : 0, 1, MPI_INFO_NULL, _comm, &ptr, memorySlot->getRecvMessageCountWindow().get());
      MPI_Win_set_errhandler(*memorySlot->getRecvMessageCountWindow(), MPI_ERRORS_RETURN);
      unlock();

      // Unfortunately, we need to do an effective realloc of the messages recv counter
      // since no modern MPI library supports MPI_Win_create over user-allocated storage anymore
      if (globalSlotProcessId[i] == _rank)
      {
        // Copying existing data over to the new storage
        *static_cast<size_t *>(ptr) = *memorySlot->getSourceLocalMemorySlot()->getMessagesRecvPointer();

        // Swapping pointers
        memorySlot->getSourceLocalMemorySlot()->getMessagesRecvPointer() = static_cast<size_t *>(ptr);
      }

      if (status != MPI_SUCCESS) HICR_THROW_RUNTIME("Failed to create MPI received message count window on exchange global memory slots.");

      // Creating MPI window for message sent count transferring
      lock();
      status = MPI_Win_allocate(globalSlotProcessId[i] == _rank ? sizeof(size_t) : 0, 1, MPI_INFO_NULL, _comm, &ptr, memorySlot->getSentMessageCountWindow().get());
      MPI_Win_set_errhandler(*memorySlot->getSentMessageCountWindow(), MPI_ERRORS_RETURN);
      unlock();

      // Unfortunately, we need to do an effective realloc of the messages sent counter
      // since no modern MPI library supports MPI_Win_create over user-allocated storage anymore
      if (globalSlotProcessId[i] == _rank)
      {
        // Copying existing data over to the new storage
        *static_cast<size_t *>(ptr) = *memorySlot->getSourceLocalMemorySlot()->getMessagesSentPointer();

        // Assigning new pointer pointers
        memorySlot->getSourceLocalMemorySlot()->getMessagesSentPointer() = static_cast<size_t *>(ptr);
      }

      if (status != MPI_SUCCESS) HICR_THROW_RUNTIME("Failed to create MPI sent message count window on exchange global memory slots.");

      // Registering global slot
      registerGlobalMemorySlot(memorySlot);
    }
  }

  /**
   * Deletes a global memory slot from the backend. This operation is collective.
   * Attempting to access the global memory slot after this operation will result in undefined behavior.
   *
   * This is not a thread-safe operation, and it is assumed that the caller has locked the mutex before calling this function.
   *
   * \param[in] memorySlotPtr Memory slot to destroy.
   */
  __INLINE__ void destroyGlobalMemorySlotImpl(std::shared_ptr<HiCR::L0::GlobalMemorySlot> memorySlotPtr) override
  {
    // Getting up-casted pointer for the execution unit
    auto memorySlot = dynamic_pointer_cast<mpi::L0::GlobalMemorySlot>(memorySlotPtr);

    // Checking whether the execution unit passed is compatible with this backend
    if (memorySlot == nullptr) HICR_THROW_LOGIC("The memory slot is not supported by this backend\n");

    auto status = MPI_Win_free(memorySlot->getDataWindow().get());
    if (status != MPI_SUCCESS) HICR_THROW_RUNTIME("On deregister global memory slot, could not free MPI data window");

    status = MPI_Win_free(memorySlot->getRecvMessageCountWindow().get());
    if (status != MPI_SUCCESS) HICR_THROW_RUNTIME("On deregister global memory slot, could not free MPI recv message count window");

    status = MPI_Win_free(memorySlot->getSentMessageCountWindow().get());
    if (status != MPI_SUCCESS) HICR_THROW_RUNTIME("On deregister global memory slot, could not free MPI sent message count window");
  }

  __INLINE__ bool acquireGlobalLockImpl(std::shared_ptr<HiCR::L0::GlobalMemorySlot> memorySlot) override
  {
    // Getting up-casted pointer for the execution unit
    auto m = dynamic_pointer_cast<mpi::L0::GlobalMemorySlot>(memorySlot);

    // Checking whether the execution unit passed is compatible with this backend
    if (m == nullptr) HICR_THROW_LOGIC("The passed memory slot is not supported by this backend\n");

    // Locking access to all relevant memory slot windows
    lockMPIWindow(m->getRank(), m->getDataWindow().get(), MPI_LOCK_EXCLUSIVE, 0);

    // Setting memory slot lock as aquired
    m->setLockAcquiredValue(true);

    // This function is assumed to always succeed
    return true;
  }

  __INLINE__ void releaseGlobalLockImpl(std::shared_ptr<HiCR::L0::GlobalMemorySlot> memorySlot) override
  {
    // Getting up-casted pointer for the execution unit
    auto m = dynamic_pointer_cast<mpi::L0::GlobalMemorySlot>(memorySlot);

    // Checking whether the execution unit passed is compatible with this backend
    if (m == nullptr) HICR_THROW_LOGIC("The passed memory slot is not supported by this backend\n");

    // Releasing access to all relevant memory slot windows
    unlockMPIWindow(m->getRank(), m->getDataWindow().get());

    // Setting memory slot lock as released
    m->setLockAcquiredValue(false);
  }

  std::shared_ptr<HiCR::L0::GlobalMemorySlot> getGlobalMemorySlotImpl(HiCR::L0::GlobalMemorySlot::tag_t tag, HiCR::L0::GlobalMemorySlot::globalKey_t globalKey) override
  {
    return nullptr;
  }
};

} // namespace HiCR::backend::mpi::L1
