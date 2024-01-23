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
#include <hicr/L1/instanceManager.hpp>
#include <backends/mpi/L0/instance.hpp>

namespace HiCR
{

namespace backend
{

namespace mpi
{

namespace L1
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
 * Tag to communicate an RPC's target
 */
#define _HICR_MPI_RPC_TAG (_HICR_MPI_INSTANCE_BASE_TAG + 1)

/**
 * Tag to communicate an RPC's result size information
 */
#define _HICR_MPI_INSTANCE_RETURN_SIZE_TAG (_HICR_MPI_INSTANCE_BASE_TAG + 2)

/**
 * Tag to communicate an RPC's result data
 */
#define _HICR_MPI_INSTANCE_RETURN_DATA_TAG (_HICR_MPI_INSTANCE_BASE_TAG + 3)

/**
 * Implementation of the HiCR MPI Instance Manager
 */
class InstanceManager final : public HiCR::L1::InstanceManager
{
  public:

  /**
   * Constructor for the MPI instance manager
   *
   * \param[in] comm The MPI subcommunicator to use in instance detection and communication
   */
  InstanceManager(MPI_Comm comm) : HiCR::L1::InstanceManager(), _comm(comm)
  {
    // Getting current rank within and size of communicator
    MPI_Comm_size(_comm, &_size);
    MPI_Comm_rank(_comm, &_rank);

    // In MPI, the initial set of processes represents all the currently available instances of HiCR
    for (int i = 0; i < _size; i++)
    {
      // Creating new MPI-based HiCR instance
      auto instance = std::make_shared<HiCR::backend::mpi::L0::Instance>(i);

      // If this is the current rank, set it as current instance
      if (i == _rank) _currentInstance = instance;

      // Adding instance to the collection
      _instances.insert(std::move(instance));
    }
  }

  ~InstanceManager() = default;

  /**
   * Triggers the execution of the specified RPC (by name) in the specified instance
   *
   * @param[in] instance The instance in which to execute the RPC
   * @param[in] RPCTargetName The name of the target RPC to execute
   *
   */
  __USED__ inline void launchRPC(HiCR::L0::Instance &instance, const std::string &RPCTargetName) const override
  {
    // Calculating hash for the RPC target's name
    auto hash = getHashFromString(RPCTargetName);

    // Getting up-casted pointer for the MPI instance
    auto MPIInstance = dynamic_cast<mpi::L0::Instance *>(&instance);

    // Checking whether the execution unit passed is compatible with this backend
    if (MPIInstance == NULL) HICR_THROW_LOGIC("The passed instance is not supported by this instance manager\n");

    // Getting rank Id for the passed instance
    const auto dest = MPIInstance->getRank();

    // Sending request
    MPI_Send(&hash, 1, MPI_UNSIGNED_LONG, dest, _HICR_MPI_RPC_TAG, _comm);
  }

  __USED__ inline void *getReturnValueImpl(HiCR::L0::Instance &instance) const override
  {
    // Getting up-casted pointer for the MPI instance
    auto MPIInstance = dynamic_cast<mpi::L0::Instance *const>(&instance);

    // Checking whether the execution unit passed is compatible with this backend
    if (MPIInstance == NULL) HICR_THROW_LOGIC("The passed instance is not supported by this instance manager\n");

    // Buffer to store the size
    size_t size = 0;

    // Getting return value size
    MPI_Recv(&size, 1, MPI_UNSIGNED_LONG, MPIInstance->getRank(), _HICR_MPI_INSTANCE_RETURN_SIZE_TAG, _comm, MPI_STATUS_IGNORE);

    // Allocating memory slot to store the return value
    auto buffer = malloc(size);

    // Getting data directly
    MPI_Recv(buffer, size, MPI_BYTE, MPIInstance->getRank(), _HICR_MPI_INSTANCE_RETURN_DATA_TAG, _comm, MPI_STATUS_IGNORE);

    // Returning memory slot containing the return value
    return buffer;
  }

  __USED__ inline void submitReturnValueImpl(const void *pointer, const size_t size) const override
  {
    // Sending message size
    MPI_Rsend(&size, 1, MPI_UNSIGNED_LONG, _RPCRequestRank, _HICR_MPI_INSTANCE_RETURN_SIZE_TAG, _comm);

    // Getting RPC execution unit index
    MPI_Rsend(pointer, size, MPI_BYTE, _RPCRequestRank, _HICR_MPI_INSTANCE_RETURN_DATA_TAG, _comm);
  }

  __USED__ inline void listenImpl() override
  {
    // We need to preserve the status to receive more information about the RPC
    MPI_Status status;

    // Storage for incoming execution unit index
    RPCTargetIndex_t rpcIdx = 0;

    // Getting RPC execution unit index
    MPI_Recv(&rpcIdx, 1, MPI_UNSIGNED_LONG, MPI_ANY_SOURCE, _HICR_MPI_RPC_TAG, _comm, &status);

    // Getting requester instance rank
    _RPCRequestRank = status.MPI_SOURCE;

    // Trying to execute RPC
    executeRPC(rpcIdx);
  }

  __USED__ inline std::shared_ptr<HiCR::L0::Instance> createInstanceImpl [[noreturn]] (const HiCR::L0::Topology &requestedTopology, int argc, char *argv[])
  {
    HICR_THROW_LOGIC("The MPI backend does not currently support the launching of new instances during runtime");
  }

  __USED__ inline virtual void finalize() override
  {
    MPI_Finalize();
  }

  private:

  /**
   * This value remembers what is the MPI rank of the instance that requested the execution of an RPC
   */
  int _RPCRequestRank = 0;

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
};

} // namespace L1

} // namespace mpi

} // namespace backend

} // namespace HiCR
