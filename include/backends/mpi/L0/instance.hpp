/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file instance.hpp
 * @brief Provides a definition for the instance class for the MPI backend
 * @author S. M. Martin
 * @date 2/11/2023
 */
#pragma once

#include <backends/mpi/L1/memoryManager.hpp>
#include <hicr/L0/instance.hpp>
#include <hicr/L0/memorySlot.hpp>
#include <mpi.h>

namespace HiCR
{

namespace backend
{

namespace mpi
{

#ifndef _HICR_MPI_INSTANCE_BASE_TAG
  /**
   * Base instance tag for data passing
   *
   * The base tag can be changed if it collides with others
   */
  #define _HICR_MPI_INSTANCE_BASE_TAG 4096
#endif

/**
 * Tag to indicate an RPC's processing unit
 */
#define _HICR_MPI_INSTANCE_PROCESSING_UNIT_TAG (_HICR_MPI_INSTANCE_BASE_TAG + 1)

/**
 * Tag to indicate an RPC's execution unit
 */
#define _HICR_MPI_INSTANCE_EXECUTION_UNIT_TAG (_HICR_MPI_INSTANCE_BASE_TAG + 2)

/**
 * Tag to indicate an RPC's result size information
 */
#define _HICR_MPI_INSTANCE_RETURN_SIZE_TAG (_HICR_MPI_INSTANCE_BASE_TAG + 3)

/**
 * Tag to indicate an RPC's result data
 */
#define _HICR_MPI_INSTANCE_RETURN_DATA_TAG (_HICR_MPI_INSTANCE_BASE_TAG + 4)

/**
 * This class represents an abstract definition for a HICR instance as represented by the MPI backend:
 */
class Instance final : public HiCR::L0::Instance
{
  public:

  /**
   * Constructor for a Instance class for the MPI backend
   * \param[in] rank The MPI rank corresponding to this HiCR instance
   * \param[in] memoryManager The MPI memory manager to use for exchanging data
   */
  Instance(const int rank, mpi::L1::MemoryManager *const memoryManager) : HiCR::L0::Instance((instanceId_t)rank),
                                                                          _memoryManager(memoryManager),
                                                                          _stateLocalMemorySlot(memoryManager->registerLocalMemorySlot(&_state, sizeof(state_t))),
                                                                          _rank(rank)
  {
  }

  /**
   * Default destructor
   */
  ~Instance() = default;

  /**
   * Retrieves this HiCR Instance's MPI rank
   * \return The MPI rank corresponding to this instance
   */
  __USED__ inline int getRank() const { return _rank; }

  __USED__ inline void execute(const processingUnitIndex_t pIdx, const executionUnitIndex_t eIdx) override
  {
    // Getting rank Id for the passed instance
    const auto dest = getRank();

    // Sending request
    MPI_Send(&eIdx, 1, MPI_UNSIGNED_LONG, dest, _HICR_MPI_INSTANCE_EXECUTION_UNIT_TAG, _memoryManager->getComm());
    MPI_Send(&pIdx, 1, MPI_UNSIGNED_LONG, dest, _HICR_MPI_INSTANCE_PROCESSING_UNIT_TAG, _memoryManager->getComm());
  }

  __USED__ inline HiCR::L0::MemorySlot *getReturnValueImpl() override
  {
    // Buffer to store the size
    size_t size;

    // Getting return value size
    MPI_Recv(&size, 1, MPI_UNSIGNED_LONG, _rank, _HICR_MPI_INSTANCE_RETURN_SIZE_TAG, _memoryManager->getComm(), MPI_STATUS_IGNORE);

    // Allocating memory slot to store the return value
    auto memorySlot = _memoryManager->allocateLocalMemorySlot(_BACKEND_MPI_DEFAULT_MEMORY_SPACE_ID, size);

    // Getting data directly
    MPI_Recv(memorySlot->getPointer(), size, MPI_BYTE, _rank, _HICR_MPI_INSTANCE_RETURN_DATA_TAG, _memoryManager->getComm(), MPI_STATUS_IGNORE);

    // Returning memory slot containing the return value
    return memorySlot;
  }

  __USED__ inline void submitReturnValueImpl(HiCR::L0::MemorySlot *value) override
  {
    // Getting return value size
    const auto size = value->getSize();

    // Getting return value data pointer
    const auto data = value->getPointer();

    // Sending message size
    MPI_Send(&size, 1, MPI_UNSIGNED_LONG, _RPCRequestRank, _HICR_MPI_INSTANCE_RETURN_SIZE_TAG, _memoryManager->getComm());

    // Getting RPC execution unit index
    MPI_Send(data, size, MPI_BYTE, _RPCRequestRank, _HICR_MPI_INSTANCE_RETURN_DATA_TAG, _memoryManager->getComm());
  }

  __USED__ inline void listenImpl() override
  {
    // Setting current state to listening
    _state = HiCR::L0::Instance::state_t::listening;

    // We need to preserve the status to receive more information about the RPC
    MPI_Status status;

    // Storage for incoming execution unit index
    HiCR::L0::Instance::executionUnitIndex_t eIdx = 0;

    // Getting RPC execution unit index
    MPI_Recv(&eIdx, 1, MPI_UNSIGNED_LONG, MPI_ANY_SOURCE, _HICR_MPI_INSTANCE_EXECUTION_UNIT_TAG, _memoryManager->getComm(), &status);

    // Getting requester instance rank
    _RPCRequestRank = status.MPI_SOURCE;

    // Storage for the index of the processing unit to use
    HiCR::L0::Instance::processingUnitIndex_t pIdx = 0;

    // Getting RPC execution unit index
    MPI_Recv(&pIdx, 1, MPI_UNSIGNED_LONG, _RPCRequestRank, _HICR_MPI_INSTANCE_PROCESSING_UNIT_TAG, _memoryManager->getComm(), MPI_STATUS_IGNORE);

    // Trying to run remote request
    runRequest(pIdx, eIdx);
  }

  /**
   * State getter
   * \return The instance's current state (up to date, even if the instance is remote)
   */
  __USED__ inline state_t getState() const override
  {
    // Obtaining state from the global memory slot, to handle the case where the instance is remote
    _memoryManager->memcpy(getStateLocalMemorySlot(), 0, getStateGlobalMemorySlot(), 0, sizeof(HiCR::L0::Instance::state_t));

    return _state;
  }

  /**
   * Gets the local memory slot storing the instance's state
   * \return A pointer to said memory slot
   */
  __USED__ inline HiCR::L0::MemorySlot *getStateLocalMemorySlot() const { return _stateLocalMemorySlot; }

  /**
   * Sets the global memory slot storing the instance's state
   * \param[in] globalSlot A pointer to the global slot to assign
   */
  __USED__ inline void setStateGlobalMemorySlot(HiCR::L0::MemorySlot *const globalSlot) { _stateGlobalMemorySlot = globalSlot; }

  /**
   * Gets the global memory slot storing the instance's state
   * \return A pointer to said memory slot
   */
  __USED__ inline HiCR::L0::MemorySlot *getStateGlobalMemorySlot() const { return _stateGlobalMemorySlot; }

  private:

  /**
   * This value remembers what is the MPI rank of the instance that requested the execution of an RPC
   */
  int _RPCRequestRank = 0;

  /**
   * Pointer to the memory manager required to obtain remote information
   */
  HiCR::backend::mpi::L1::MemoryManager *const _memoryManager;

  /**
   * Local memory slot that represents the instance status
   */
  HiCR::L0::MemorySlot *const _stateLocalMemorySlot;

  /**
   * Global memory slot that represents the instance status
   */
  HiCR::L0::MemorySlot *_stateGlobalMemorySlot;

  /**
   * Remembers the MPI rank this instance belongs to
   */
  const int _rank;
};

} // namespace mpi

} // namespace backend

} // namespace HiCR
