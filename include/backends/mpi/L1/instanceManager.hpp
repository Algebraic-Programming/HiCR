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

#include <memory>
#include <mpi.h>
#include <hicr/definitions.hpp>
#include <hicr/L0/topology.hpp>
#include <hicr/L0/memorySpace.hpp>
#include <hicr/L1/instanceManager.hpp>
#include <hicr/L1/memoryManager.hpp>
#include <hicr/L1/computeManager.hpp>
#include <backends/mpi/L0/instance.hpp>
#include <backends/mpi/L1/communicationManager.hpp>

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
   * \param[in] memoryManager The memory manager to use for buffer allocations
   * \param[in] communicationManager The communication manager to use for internal data passing
   * \param[in] computeManager The compute manager to use for RPC running
   * \param[in] bufferMemorySpace The memory space from which to allocate data buffers
   */
  InstanceManager(std::shared_ptr<HiCR::backend::mpi::L1::CommunicationManager> communicationManager,
                  std::shared_ptr<HiCR::L1::ComputeManager> computeManager,
                  std::shared_ptr<HiCR::L1::MemoryManager> memoryManager) : HiCR::L1::InstanceManager(communicationManager, computeManager, memoryManager),
                                                                              _MPICommunicationManager(communicationManager)
  {
    // Checking whether the execution unit passed is compatible with this backend
    if (_MPICommunicationManager == NULL) HICR_THROW_LOGIC("The passed memory manager is not supported by this instance manager\n");

    // In MPI, the initial set of processes represents all the currently available instances of HiCR
    for (int i = 0; i < _MPICommunicationManager->getSize(); i++)
    {
      // Creating new MPI-based HiCR instance
      auto instance = std::make_shared<HiCR::backend::mpi::L0::Instance>(i);

      // If this is the current rank, set it as current instance
      if (i == _MPICommunicationManager->getRank()) _currentInstance = instance;

      // Adding instance to the collection
      _instances.insert(std::move(instance));
    }
  }

  ~InstanceManager() = default;

  __USED__ inline void execute(HiCR::L0::Instance &instance, const processingUnitIndex_t pIdx, const executionUnitIndex_t eIdx) const override
  {
    // Getting up-casted pointer for the MPI instance
    auto MPIInstance = dynamic_cast<mpi::L0::Instance *>(&instance);

    // Checking whether the execution unit passed is compatible with this backend
    if (MPIInstance == NULL) HICR_THROW_LOGIC("The passed instance is not supported by this instance manager\n");

    // Getting rank Id for the passed instance
    const auto dest = MPIInstance->getRank();

    // Sending request
    MPI_Send(&eIdx, 1, MPI_UNSIGNED_LONG, dest, _HICR_MPI_INSTANCE_EXECUTION_UNIT_TAG, _MPICommunicationManager->getComm());
    MPI_Send(&pIdx, 1, MPI_UNSIGNED_LONG, dest, _HICR_MPI_INSTANCE_PROCESSING_UNIT_TAG, _MPICommunicationManager->getComm());
  }

  __USED__ inline std::shared_ptr<HiCR::L0::LocalMemorySlot> getReturnValueImpl(HiCR::L0::Instance &instance) const override
  {
    // Getting up-casted pointer for the MPI instance
    auto MPIInstance = dynamic_cast<mpi::L0::Instance *const>(&instance);

    // Checking whether the execution unit passed is compatible with this backend
    if (MPIInstance == NULL) HICR_THROW_LOGIC("The passed instance is not supported by this instance manager\n");

    // Buffer to store the size
    size_t size = 0;

    // Getting return value size
    MPI_Recv(&size, 1, MPI_UNSIGNED_LONG, MPIInstance->getRank(), _HICR_MPI_INSTANCE_RETURN_SIZE_TAG, _MPICommunicationManager->getComm(), MPI_STATUS_IGNORE);

    // Allocating memory slot to store the return value
    auto memorySlot = _memoryManager->allocateLocalMemorySlot(_bufferMemorySpace, size);

    // Getting data directly
    MPI_Recv(memorySlot->getPointer(), size, MPI_BYTE, MPIInstance->getRank(), _HICR_MPI_INSTANCE_RETURN_DATA_TAG, _MPICommunicationManager->getComm(), MPI_STATUS_IGNORE);

    // Returning memory slot containing the return value
    return memorySlot;
  }

  __USED__ inline void submitReturnValueImpl(std::shared_ptr<HiCR::L0::LocalMemorySlot> value) const override
  {
    // Getting return value size
    const auto size = value->getSize();

    // Getting return value data pointer
    const auto data = value->getPointer();

    // Sending message size
    MPI_Rsend(&size, 1, MPI_UNSIGNED_LONG, _RPCRequestRank, _HICR_MPI_INSTANCE_RETURN_SIZE_TAG, _MPICommunicationManager->getComm());

    // Getting RPC execution unit index
    MPI_Rsend(data, size, MPI_BYTE, _RPCRequestRank, _HICR_MPI_INSTANCE_RETURN_DATA_TAG, _MPICommunicationManager->getComm());
  }

  __USED__ inline void listenImpl() override
  {
    // We need to preserve the status to receive more information about the RPC
    MPI_Status status;

    // Storage for incoming execution unit index
    executionUnitIndex_t eIdx = 0;

    // Getting RPC execution unit index
    MPI_Recv(&eIdx, 1, MPI_UNSIGNED_LONG, MPI_ANY_SOURCE, _HICR_MPI_INSTANCE_EXECUTION_UNIT_TAG, _MPICommunicationManager->getComm(), &status);

    // Getting requester instance rank
    _RPCRequestRank = status.MPI_SOURCE;

    // Storage for the index of the processing unit to use
    processingUnitIndex_t pIdx = 0;

    // Getting RPC execution unit index
    MPI_Recv(&pIdx, 1, MPI_UNSIGNED_LONG, _RPCRequestRank, _HICR_MPI_INSTANCE_PROCESSING_UNIT_TAG, _MPICommunicationManager->getComm(), MPI_STATUS_IGNORE);

    // Trying to run remote request
    runRequest(pIdx, eIdx);
  }

  __USED__ inline std::shared_ptr<HiCR::L0::Instance> createInstanceImpl (const HiCR::L0::Topology& requestedTopology)
  {
    // The MPI backend does not currently support the launching of new instances during runtime"
    return nullptr;
  }

  private:

  /**
   * This value remembers what is the MPI rank of the instance that requested the execution of an RPC
   */
  int _RPCRequestRank = 0;

  /**
   * Internal communication manager for MPI
   */
  const std::shared_ptr<mpi::L1::CommunicationManager> _MPICommunicationManager;
};

} // namespace L1

} // namespace mpi

} // namespace backend

} // namespace HiCR
