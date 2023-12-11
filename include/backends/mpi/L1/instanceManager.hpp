/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file instanceManager.hpp
 * @brief This file implements the instance manager class for the MPI backend
 * @author S. M. Martin
 * @date 2/11/2023
 */

#pragma once

#include <backends/mpi/L0/instance.hpp>
#include <backends/mpi/L1/memoryManager.hpp>
#include <hicr/L1/instanceManager.hpp>
#include <hicr/common/definitions.hpp>
#include <mpi.h>

namespace HiCR
{

namespace backend
{

namespace mpi
{

namespace L1
{

/**
 * Instance manager tag for exchanging memory slots
 */
#define _HICR_MPI_INSTANCE_MANAGER_TAG 4096

/**
 * Implementation of the HiCR MPI Instance Manager
 */
class InstanceManager final : public HiCR::L1::InstanceManager
{
  public:

  /**
   * Constructor for the MPI instance manager
   *
   * \param[in] memoryManager The memory manager to use for internal data passing
   */
  InstanceManager(HiCR::L1::MemoryManager *const memoryManager) : HiCR::L1::InstanceManager(memoryManager), _MPIMemoryManager(dynamic_cast<mpi::L1::MemoryManager *const>(memoryManager))
  {
    // Checking whether the execution unit passed is compatible with this backend
    if (_MPIMemoryManager == NULL) HICR_THROW_LOGIC("The passed memory manager is not supported by this instance manager\n");

    // In MPI, the initial set of processes represents all the currently available instances of HiCR
    for (int i = 0; i < _MPIMemoryManager->getSize(); i++)
    {
      // Creating new MPI-based HiCR instance
      auto instance = new HiCR::backend::mpi::L0::Instance(i, _MPIMemoryManager);

      // If this is the current rank, set it as current instance
      if (i == _MPIMemoryManager->getRank()) _currentInstance = instance;

      // Adding instance to the collection
      _instances.insert(instance);
    }

    // Getting MPI-specific current instance pointer
    auto currentInstanceMPIPtr = (mpi::L0::Instance *)_currentInstance;

    // Exchanging global slots for instance state values
    _MPIMemoryManager->exchangeGlobalMemorySlots(_HICR_MPI_INSTANCE_MANAGER_TAG, {std::make_pair(currentInstanceMPIPtr->getRank(), currentInstanceMPIPtr->getStateLocalMemorySlot())});

    // Getting globally exchanged slots
    for (auto instance : _instances)
    {
      // Getting MPI-specific instance pointer
      auto instanceMPIPtr = (mpi::L0::Instance *)instance;

      // Getting global slot pointer
      auto globalSlot = _MPIMemoryManager->getGlobalMemorySlot(_HICR_MPI_INSTANCE_MANAGER_TAG, instanceMPIPtr->getRank());

      // Assigning it to the corresponding instance
      instanceMPIPtr->setStateGlobalMemorySlot(globalSlot);
    }
  }

  ~InstanceManager() = default;

  __USED__ inline void execute(HiCR::L0::Instance *instance, const processingUnitIndex_t pIdx, const executionUnitIndex_t eIdx) const override
  {
    // Getting up-casted pointer for the MPI instance
    auto MPIInstance = dynamic_cast<mpi::L0::Instance *const>(instance);

    // Checking whether the execution unit passed is compatible with this backend
    if (MPIInstance == NULL) HICR_THROW_LOGIC("The passed instance is not supported by this instance manager\n");

    // Getting rank Id for the passed instance
    const auto dest = MPIInstance->getRank();

    // Sending request
    MPI_Send(&eIdx, 1, MPI_UNSIGNED_LONG, dest, _HICR_MPI_INSTANCE_EXECUTION_UNIT_TAG, _MPIMemoryManager->getComm());
    MPI_Send(&pIdx, 1, MPI_UNSIGNED_LONG, dest, _HICR_MPI_INSTANCE_PROCESSING_UNIT_TAG, _MPIMemoryManager->getComm());
  }

  __USED__ inline HiCR::L0::MemorySlot *getReturnValueImpl(HiCR::L0::Instance *instance) const override
  {
    // Getting up-casted pointer for the MPI instance
    auto MPIInstance = dynamic_cast<mpi::L0::Instance *const>(instance);

    // Checking whether the execution unit passed is compatible with this backend
    if (MPIInstance == NULL) HICR_THROW_LOGIC("The passed instance is not supported by this instance manager\n");

    // Buffer to store the size
    size_t size = 0;

    // Getting return value size
    MPI_Recv(&size, 1, MPI_UNSIGNED_LONG, MPIInstance->getRank(), _HICR_MPI_INSTANCE_RETURN_SIZE_TAG, _MPIMemoryManager->getComm(), MPI_STATUS_IGNORE);

    // Allocating memory slot to store the return value
    auto memorySlot = _memoryManager->allocateLocalMemorySlot(_BACKEND_MPI_DEFAULT_MEMORY_SPACE_ID, size);

    // Getting data directly
    MPI_Recv(memorySlot->getPointer(), size, MPI_BYTE, MPIInstance->getRank(), _HICR_MPI_INSTANCE_RETURN_DATA_TAG, _MPIMemoryManager->getComm(), MPI_STATUS_IGNORE);

    // Returning memory slot containing the return value
    return memorySlot;
  }

  __USED__ inline void submitReturnValueImpl(HiCR::L0::MemorySlot *value) const override
  {
    // Getting return value size
    const auto size = value->getSize();

    // Getting return value data pointer
    const auto data = value->getPointer();

    // Sending message size
    MPI_Send(&size, 1, MPI_UNSIGNED_LONG, _RPCRequestRank, _HICR_MPI_INSTANCE_RETURN_SIZE_TAG, _MPIMemoryManager->getComm());

    // Getting RPC execution unit index
    MPI_Send(data, size, MPI_BYTE, _RPCRequestRank, _HICR_MPI_INSTANCE_RETURN_DATA_TAG, _MPIMemoryManager->getComm());
  }

  __USED__ inline void listenImpl() override
  {
    // We need to preserve the status to receive more information about the RPC
    MPI_Status status;

    // Storage for incoming execution unit index
    executionUnitIndex_t eIdx = 0;

    // Getting RPC execution unit index
    MPI_Recv(&eIdx, 1, MPI_UNSIGNED_LONG, MPI_ANY_SOURCE, _HICR_MPI_INSTANCE_EXECUTION_UNIT_TAG, _MPIMemoryManager->getComm(), &status);

    // Getting requester instance rank
    _RPCRequestRank = status.MPI_SOURCE;

    // Storage for the index of the processing unit to use
    processingUnitIndex_t pIdx = 0;

    // Getting RPC execution unit index
    MPI_Recv(&pIdx, 1, MPI_UNSIGNED_LONG, _RPCRequestRank, _HICR_MPI_INSTANCE_PROCESSING_UNIT_TAG, _MPIMemoryManager->getComm(), MPI_STATUS_IGNORE);

    // Trying to run remote request
    runRequest(pIdx, eIdx);
  }

  private:

  /**
   * This value remembers what is the MPI rank of the instance that requested the execution of an RPC
   */
  int _RPCRequestRank = 0;

  /**
   * Internal pointer for a casted MPI memory manager
   */
  mpi::L1::MemoryManager *const _MPIMemoryManager;
};

} // namespace L1

} // namespace mpi

} // namespace backend

} // namespace HiCR
