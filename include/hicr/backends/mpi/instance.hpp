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

#include <mpi.h>
#include <hicr/instance.hpp>
#include <hicr/memorySlot.hpp>
#include <hicr/backends/mpi/memoryManager.hpp>

namespace HiCR
{

namespace backend
{

namespace mpi
{

// The base tag can be changed if it collides with others
#ifndef _HICR_MPI_INSTANCE_BASE_TAG
#define _HICR_MPI_INSTANCE_BASE_TAG 4096
#endif
#define _HICR_MPI_INSTANCE_PROCESSING_UNIT_TAG (_HICR_MPI_INSTANCE_BASE_TAG+1)
#define _HICR_MPI_INSTANCE_EXECUTION_UNIT_TAG  (_HICR_MPI_INSTANCE_BASE_TAG+2)
#define _HICR_MPI_INSTANCE_RETURN_SIZE_TAG     (_HICR_MPI_INSTANCE_BASE_TAG+3)
#define _HICR_MPI_INSTANCE_RETURN_DATA_TAG     (_HICR_MPI_INSTANCE_BASE_TAG+4)

/**
 * This class represents an abstract definition for a HICR instance as represented by the MPI backend:
 */
class Instance final : public HiCR::Instance
{
  public:

  /**
   * Constructor for a Instance class for the MPI backend
   */
  Instance(const int rank, const MPI_Comm comm, mpi::MemoryManager* const memoryManager) :
   _memoryManager(memoryManager),
   _stateLocalMemorySlot(memoryManager->registerLocalMemorySlot(&_state, sizeof(state_t))),
   _rank(rank),
   _comm(comm)
  { }

  /**
   * Default destructor
   */
  ~Instance() = default;

  __USED__ inline int getRank() const { return _rank; }

  __USED__ inline void execute(const processingUnitIndex_t pIdx, const executionUnitIndex_t eIdx) override
  {
   // Getting rank Id for the passed instance
   const auto dest = getRank();

   // Sending request
   MPI_Send(&eIdx, 1, MPI_UNSIGNED_LONG, dest, _HICR_MPI_INSTANCE_EXECUTION_UNIT_TAG, _comm);
   MPI_Send(&pIdx, 1, MPI_UNSIGNED_LONG, dest, _HICR_MPI_INSTANCE_PROCESSING_UNIT_TAG, _comm);
  }

  __USED__ inline size_t getReturnValueSizeImpl() override
  {
   // Buffer to store the size
   size_t size;

   // Getting return value size
   MPI_Recv(&size, 1, MPI_UNSIGNED_LONG, _rank, _HICR_MPI_INSTANCE_RETURN_SIZE_TAG, _comm, MPI_STATUS_IGNORE);

   // Returning value
   return size;
  }

  __USED__ inline void getReturnValueDataImpl(uint8_t* data, const size_t size) override
  {
   // Getting data directly
   MPI_Recv(data, size, MPI_BYTE, _rank, _HICR_MPI_INSTANCE_RETURN_DATA_TAG, _comm, MPI_STATUS_IGNORE);
  }

  __USED__ inline void submitReturnValueImpl(const uint8_t* data, const size_t size) override
  {
   // Sending message size
   MPI_Send(&size, 1, MPI_UNSIGNED_LONG, _RPCRequestRank, _HICR_MPI_INSTANCE_RETURN_SIZE_TAG, _comm);

   // Getting RPC execution unit index
   MPI_Send(data, size, MPI_BYTE, _RPCRequestRank, _HICR_MPI_INSTANCE_RETURN_DATA_TAG, _comm);
  }

  __USED__ inline void listenImpl() override
  {
   // Setting current state to listening
   _state = HiCR::Instance::state_t::listening;

   // We need to preserve the status to receive more information about the RPC
   MPI_Status status;

   // Storage for incoming execution unit index
   HiCR::Instance::executionUnitIndex_t eIdx = 0;

   // Getting RPC execution unit index
   MPI_Recv(&eIdx, 1, MPI_UNSIGNED_LONG, MPI_ANY_SOURCE, _HICR_MPI_INSTANCE_EXECUTION_UNIT_TAG, _comm, &status);

   // Getting requester instance rank
   _RPCRequestRank = status.MPI_SOURCE;

   // Storage for the index of the processing unit to use
   HiCR::Instance::processingUnitIndex_t pIdx = 0;

   // Getting RPC execution unit index
   MPI_Recv(&pIdx, 1, MPI_UNSIGNED_LONG, _RPCRequestRank, _HICR_MPI_INSTANCE_PROCESSING_UNIT_TAG, _comm, MPI_STATUS_IGNORE);

   // Trying to run remote request
   runRequest(pIdx, eIdx);
  }


  /**
   * State getter
   */
  __USED__ inline state_t getState() const override
  {
   // Obtaining state from the global memory slot, to handle the case where the instance is remote
   _memoryManager->memcpy(getStateLocalMemorySlot(), 0, getStateGlobalMemorySlot(), 0, sizeof(HiCR::Instance::state_t));

   return _state;
  }

  __USED__ inline HiCR::MemorySlot* getStateLocalMemorySlot() const { return _stateLocalMemorySlot; }
  __USED__ inline void setStateGlobalMemorySlot(HiCR::MemorySlot* const globalSlot) { _stateGlobalMemorySlot = globalSlot; }
  __USED__ inline HiCR::MemorySlot* getStateGlobalMemorySlot() const { return _stateGlobalMemorySlot; }

  private:

  /**
   * This value remembers what is the MPI rank of the instance that requested the execution of an RPC
   */
  int _RPCRequestRank = 0;

  /**
   * Pointer to the memory manager required to obtain remote information
   */
  HiCR::backend::MemoryManager* const _memoryManager;

  /**
   * Local memory slot that represents the instance status
   */
  HiCR::MemorySlot* const _stateLocalMemorySlot;

  /**
   * Global memory slot that represents the instance status
   */
  HiCR::MemorySlot* _stateGlobalMemorySlot;

  /**
   * Remembers the MPI rank this instance belongs to
   */
  const int _rank;

  /**
   * Remembers the MPI communicator this rank belongs to
   */
  const MPI_Comm _comm;
};

} // namespace mpi

} // namespace backend

} // namespace HiCR
