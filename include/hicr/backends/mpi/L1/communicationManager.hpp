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
#include <hicr/core/definitions.hpp>
#include <hicr/core/L0/localMemorySlot.hpp>
#include <hicr/core/L1/communicationManager.hpp>
#include "../L0/localMemorySlot.hpp"
#include "../L0/globalMemorySlot.hpp"

namespace HiCR
{

namespace backend
{

namespace mpi
{

namespace L1
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

  ~CommunicationManager() = default;

  /**
   * MPI Communicator getter
   * \return The internal MPI communicator used during the instantiation of this class
   */
  const MPI_Comm getComm() const { return _comm; }

  /**
   * MPI Communicator size getter
   * \return The size of the internal MPI communicator used during the instantiation of this class
   */
  const int getSize() const { return _size; }

  /**
   * MPI Communicator rank getter
   * \return The rank within the internal MPI communicator used during the instantiation of this class that corresponds to this instance
   */
  const int getRank() const { return _rank; }

  /**
   * Blocking mutex lock on the communication manager mutex
   */
  void lockMPICalls() { _mutex.lock(); }

  /**
   * Mutex unlock on the communication manager mutex
   */
  void unlockMPICalls() { _mutex.unlock(); }

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

  __INLINE__ void lockMPIWindow(int rank, MPI_Win *window, int MPILockType, int MPIAssert)
  {
    // Locking MPI window to ensure the messages arrives before returning
    int mpiStatus;
    do {
      this->lockMPICalls();
      mpiStatus = MPI_Win_lock(MPILockType, rank, MPIAssert, *window) != MPI_SUCCESS;
      this->unlockMPICalls();
    }
    while (mpiStatus != MPI_SUCCESS);
  }

  __INLINE__ void unlockMPIWindow(int rank, MPI_Win *window)
  {
    // Unlocking window after copy is completed
    int mpiStatus;
    do {
      this->lockMPICalls();
      mpiStatus = MPI_Win_unlock(rank, *window) != MPI_SUCCESS;
      this->unlockMPICalls();
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
    const size_t one = 1;
    size_t       value;

    // There is no datatype in MPI for size_t (the counters), but
    // MPI_AINT is supposed to be large enough and portable
    this->lockMPICalls();
    auto status = MPI_Fetch_and_op(&one, &value, MPI_AINT, rank, 0, MPI_SUM, *window);
    this->unlockMPICalls();

    // Checking execution status
    if (status != MPI_SUCCESS)
      HICR_THROW_RUNTIME("Failed to increase remote message counter (on operation: MPI_Put) for rank %d, MPI Window pointer 0x%lX", rank, (uint64_t)window);

    // Unlocking window after copy is completed
    unlockMPIWindow(rank, window);
  }

  __INLINE__ void memcpyImpl(std::shared_ptr<HiCR::L0::LocalMemorySlot>  destinationSlot,
                             size_t                                      dst_offset,
                             std::shared_ptr<HiCR::L0::GlobalMemorySlot> sourceSlotPtr,
                             size_t                                      sourceOffset,
                             size_t                                      size) override
  {
    // Getting up-casted pointer for the execution unit
    auto source = dynamic_pointer_cast<mpi::L0::GlobalMemorySlot>(sourceSlotPtr);

    // Checking whether the execution unit passed is compatible with this backend
    if (source == NULL) HICR_THROW_LOGIC("The passed source memory slot is not supported by this backend\n");

    // Getting ranks for the involved processes
    const auto sourceRank = source->getRank();

    // Check if we already acquired a lock on the memory slots
    bool isSourceSlotLockAcquired = source->getLockAcquiredValue();

    // Calculating pointer
    auto destinationPointer = (void *)(((uint8_t *)destinationSlot->getPointer()) + dst_offset);

    // Getting data window for the involved processes
    auto sourceDataWindow = source->getDataWindow().get();

    // Getting recv message count window for the involved processes
    auto sourceSentMessageWindow = source->getSentMessageCountWindow().get();

    // Locking MPI window to ensure the messages arrives before returning. This will not exclude other processes from accessing the data (MPI_LOCK_SHARED)
    if (isSourceSlotLockAcquired == false) lockMPIWindow(sourceRank, sourceDataWindow, MPI_LOCK_SHARED, MPI_MODE_NOCHECK);

    // Executing the get operation
    {
      this->lockMPICalls();
      auto status = MPI_Get(destinationPointer, size, MPI_BYTE, sourceRank, sourceOffset, size, MPI_BYTE, *sourceDataWindow);
      this->unlockMPICalls();
      if (status != MPI_SUCCESS) HICR_THROW_RUNTIME("Failed to run MPI_Get");
    }

    // Making sure the operation finished
    {
      this->lockMPICalls();
      auto status = MPI_Win_flush(sourceRank, *sourceDataWindow);
      this->unlockMPICalls();
      if (status != MPI_SUCCESS) HICR_THROW_RUNTIME("Failed to run MPI_Win_flush");
    }

    // Unlocking window, if taken, after copy is completed
    if (isSourceSlotLockAcquired == false) unlockMPIWindow(sourceRank, sourceDataWindow);

    // Increasing the remote sent message counter and local destination received message counter
    increaseWindowCounter(sourceRank, sourceSentMessageWindow);
    destinationSlot->increaseMessagesRecv();
  }

  __INLINE__ void memcpyImpl(std::shared_ptr<HiCR::L0::GlobalMemorySlot> destinationSlotPtr,
                             size_t                                      dst_offset,
                             std::shared_ptr<HiCR::L0::LocalMemorySlot>  sourceSlot,
                             size_t                                      sourceOffset,
                             size_t                                      size) override
  {
    // Getting up-casted pointer for the execution unit
    auto destination = dynamic_pointer_cast<mpi::L0::GlobalMemorySlot>(destinationSlotPtr);

    // Checking whether the execution unit passed is compatible with this backend
    if (destination == NULL) HICR_THROW_LOGIC("The passed destination memory slot is not supported by this backend\n");

    // Getting ranks for the involved processes
    const auto destinationRank = destination->getRank();

    // Check if we already acquired a lock on the memory slots
    bool isDestinationSlotLockAcquired = destination->getLockAcquiredValue();

    // Calculating pointers
    auto sourcePointer = (void *)(((uint8_t *)sourceSlot->getPointer()) + sourceOffset);

    // Getting data window for the involved processes
    auto destinationDataWindow = destination->getDataWindow().get();

    // Getting recv message count windows for the involved process
    auto destinationRecvMessageWindow = destination->getRecvMessageCountWindow().get();

    // Locking MPI window to ensure the messages arrives before returning. This will not exclude other processes from accessing the data (MPI_LOCK_SHARED)
    if (isDestinationSlotLockAcquired == false) lockMPIWindow(destinationRank, destinationDataWindow, MPI_LOCK_SHARED, MPI_MODE_NOCHECK);

    // Executing the put operation
    {
      this->lockMPICalls();
      auto status = MPI_Put(sourcePointer, size, MPI_BYTE, destinationRank, dst_offset, size, MPI_BYTE, *destinationDataWindow);
      this->unlockMPICalls();
      if (status != MPI_SUCCESS) HICR_THROW_RUNTIME("Failed to run data MPI_Put");
    }

    // Making sure the operation finished
    {
      this->lockMPICalls();
      auto status = MPI_Win_flush(destinationRank, *destinationDataWindow);
      this->unlockMPICalls();
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
   * Implementation of the fence operation for the mpi backend.
   * A barrier is sufficient, as MPI_Win_lock/MPI_Win_unlock passive
   * synchronization is used to transfer data
   */
  __INLINE__ void fenceImpl(HiCR::L0::GlobalMemorySlot::tag_t tag) override { MPI_Barrier(_comm); }

  __INLINE__ void exchangeGlobalMemorySlotsImpl(HiCR::L0::GlobalMemorySlot::tag_t tag, const std::vector<globalKeyMemorySlotPair_t> &memorySlots) override
  {
    // Obtaining local slots to exchange
    int localSlotCount = (int)memorySlots.size();

    // Obtaining the local slots to exchange per process in the communicator
    std::vector<int> perProcessSlotCount(_size);
    this->lockMPICalls();
    MPI_Allgather(&localSlotCount, 1, MPI_INT, perProcessSlotCount.data(), 1, MPI_INT, _comm);
    this->unlockMPICalls();

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
    this->lockMPICalls();
    MPI_Allgatherv(
      localSlotSizes.data(), localSlotCount, MPI_UNSIGNED_LONG, globalSlotSizes.data(), perProcessSlotCount.data(), perProcessSlotOffsets.data(), MPI_UNSIGNED_LONG, _comm);
    MPI_Allgatherv(
      localSlotKeys.data(), localSlotCount, MPI_UNSIGNED_LONG, globalSlotKeys.data(), perProcessSlotCount.data(), perProcessSlotOffsets.data(), MPI_UNSIGNED_LONG, _comm);
    MPI_Allgatherv(localSlotProcessId.data(), localSlotCount, MPI_INT, globalSlotProcessId.data(), perProcessSlotCount.data(), perProcessSlotOffsets.data(), MPI_INT, _comm);
    this->unlockMPICalls();

    // Now also creating pointer vector to remember local pointers, when required for memcpys
    std::vector<void **>                                    globalSlotPointers(globalSlotCount);
    std::vector<std::shared_ptr<HiCR::L0::LocalMemorySlot>> globalSourceSlots(globalSlotCount);
    size_t                                                  localPointerPos = 0;
    for (size_t i = 0; i < globalSlotPointers.size(); i++)
    {
      // If the rank associated with this slot is remote, don't store the pointer, otherwise store it.
      if (globalSlotProcessId[i] != _rank)
      {
        globalSlotPointers[i] = NULL;
        globalSourceSlots[i]  = NULL;
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
      this->lockMPICalls();
      auto status = MPI_Win_allocate(globalSlotProcessId[i] == _rank ? globalSlotSizes[i] : 0, 1, MPI_INFO_NULL, _comm, &ptr, memorySlot->getDataWindow().get());
      MPI_Win_set_errhandler(*memorySlot->getDataWindow(), MPI_ERRORS_RETURN);
      this->unlockMPICalls();

      // Unfortunately, we need to do an effective duplucation of the original local memory slot storage
      // since no modern MPI library supports MPI_Win_create over user-allocated storage anymore
      if (globalSlotProcessId[i] == _rank)
      {
        // Copying existing data over to the new storage
        std::memcpy(ptr, *(globalSlotPointers[i]), globalSlotSizes[i]);

        // Freeing up memory
        this->lockMPICalls();
        MPI_Free_mem(*(globalSlotPointers[i]));
        this->unlockMPICalls();

        // Swapping pointers
        *(globalSlotPointers[i]) = ptr;
      }

      if (status != MPI_SUCCESS) HICR_THROW_RUNTIME("Failed to create MPI data window on exchange global memory slots.");

      // Creating MPI window for message received count transferring
      this->lockMPICalls();
      status = MPI_Win_allocate(globalSlotProcessId[i] == _rank ? sizeof(size_t) : 0, 1, MPI_INFO_NULL, _comm, &ptr, memorySlot->getRecvMessageCountWindow().get());
      MPI_Win_set_errhandler(*memorySlot->getRecvMessageCountWindow(), MPI_ERRORS_RETURN);
      this->unlockMPICalls();

      // Unfortunately, we need to do an effective realloc of the messages recv counter
      // since no modern MPI library supports MPI_Win_create over user-allocated storage anymore
      if (globalSlotProcessId[i] == _rank)
      {
        // Copying existing data over to the new storage
        *(size_t *)ptr = *memorySlot->getSourceLocalMemorySlot()->getMessagesRecvPointer();

        // Swapping pointers
        memorySlot->getSourceLocalMemorySlot()->getMessagesRecvPointer() = (size_t *)ptr;
      }

      if (status != MPI_SUCCESS) HICR_THROW_RUNTIME("Failed to create MPI received message count window on exchange global memory slots.");

      // Creating MPI window for message sent count transferring
      this->lockMPICalls();
      status = MPI_Win_allocate(globalSlotProcessId[i] == _rank ? sizeof(size_t) : 0, 1, MPI_INFO_NULL, _comm, &ptr, memorySlot->getSentMessageCountWindow().get());
      MPI_Win_set_errhandler(*memorySlot->getSentMessageCountWindow(), MPI_ERRORS_RETURN);
      this->unlockMPICalls();

      // Unfortunately, we need to do an effective realloc of the messages sent counter
      // since no modern MPI library supports MPI_Win_create over user-allocated storage anymore
      if (globalSlotProcessId[i] == _rank)
      {
        // Copying existing data over to the new storage
        *(size_t *)ptr = *memorySlot->getSourceLocalMemorySlot()->getMessagesSentPointer();

        // Assigning new pointer pointers
        memorySlot->getSourceLocalMemorySlot()->getMessagesSentPointer() = (size_t *)ptr;
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
   * \param[in] memorySlotPtr Memory slot to destroy.
   */
  __INLINE__ void destroyGlobalMemorySlotImpl(std::shared_ptr<HiCR::L0::GlobalMemorySlot> memorySlotPtr) override
  {
    // Getting up-casted pointer for the execution unit
    auto memorySlot = dynamic_pointer_cast<mpi::L0::GlobalMemorySlot>(memorySlotPtr);

    // Checking whether the execution unit passed is compatible with this backend
    if (memorySlot == NULL) HICR_THROW_LOGIC("The memory slot is not supported by this backend\n");

    this->lockMPICalls();

    auto status = MPI_Win_free(memorySlot->getDataWindow().get());
    if (status != MPI_SUCCESS) HICR_THROW_RUNTIME("On deregister global memory slot, could not free MPI data window");

    status = MPI_Win_free(memorySlot->getRecvMessageCountWindow().get());
    if (status != MPI_SUCCESS) HICR_THROW_RUNTIME("On deregister global memory slot, could not free MPI recv message count window");

    status = MPI_Win_free(memorySlot->getSentMessageCountWindow().get());
    if (status != MPI_SUCCESS) HICR_THROW_RUNTIME("On deregister global memory slot, could not free MPI sent message count window");

    this->unlockMPICalls();
  }

  __INLINE__ bool acquireGlobalLockImpl(std::shared_ptr<HiCR::L0::GlobalMemorySlot> memorySlot) override
  {
    // Getting up-casted pointer for the execution unit
    auto m = dynamic_pointer_cast<mpi::L0::GlobalMemorySlot>(memorySlot);

    // Checking whether the execution unit passed is compatible with this backend
    if (m == NULL) HICR_THROW_LOGIC("The passed memory slot is not supported by this backend\n");

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
    if (m == NULL) HICR_THROW_LOGIC("The passed memory slot is not supported by this backend\n");

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

} // namespace L1

} // namespace mpi

} // namespace backend

} // namespace HiCR
