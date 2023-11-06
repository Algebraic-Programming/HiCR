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

#include <mpi.h>
#include <hicr/common/definitions.hpp>
#include <hicr/backends/instanceManager.hpp>
#include <hicr/backends/mpi/instance.hpp>

namespace HiCR
{

namespace backend
{

namespace mpi
{

#define _HICR_MPI_ROOT_RANK 0
#define _HICR_MPI_COORDINATION_TAG 4096

/**
 * Implementation of the HiCR MPI Instance Manager
 */
class InstanceManager final : public HiCR::backend::InstanceManager
{
  public:

  /**
   * Constructor for the MPI instance manager
   *
   * \param[in] comm The MPI subcommunicator to use in the communication operations in this backend.
   * If not specified, it will use MPI_COMM_WORLD
   */
  InstanceManager(MPI_Comm comm = MPI_COMM_WORLD) : _comm(comm)
  {
    // Obtaining process count and my own rank id
    MPI_Comm_size(_comm, &_size);
    MPI_Comm_rank(_comm, &_rank);

    // In MPI, the initial set of processes represents all the currently available instances of HiCR
    for (int i = 0; i < _size; i++)
    {
      // Creating new MPI-based HiCR instance
      auto instance = std::make_unique<HiCR::backend::mpi::Instance>(i);

      // If this is the root rank, setting it as running
      if (i == _HICR_MPI_ROOT_RANK) instance->setState(HiCR::Instance::state_t::running);

      // Adding instance to the collection
      _instances.insert(std::move(instance));
    }
  }

  ~InstanceManager() = default;

  __USED__ inline bool isCoordinatorInstance() override
  {
   // For the MPI backend, the coordinator instance will be set to process with rank _HICR_MPI_ROOT_RANK
   if (_rank == _HICR_MPI_ROOT_RANK) return true;
   return false;
  }

  __USED__ inline void listen() override
  {
   // We need to preserve the status to receive more information about the RPC
   MPI_Status status;

   // Storage for incoming execution unit index
   executionUnitIndex_t eIdx = 0;

   // Getting RPC execution unit index
   MPI_Recv(&eIdx, 1, MPI_UNSIGNED_LONG, MPI_ANY_SOURCE, _HICR_MPI_COORDINATION_TAG, _comm, &status);

   // Getting sender rank
   int sender = status.MPI_SOURCE;

   // Storage for processing unit index to use
   processingUnitIndex_t pIdx = 0;

   // Getting RPC execution unit index
   MPI_Recv(&pIdx, 1, MPI_UNSIGNED_LONG, sender, _HICR_MPI_COORDINATION_TAG, _comm, MPI_STATUS_IGNORE);

   // Trying to run remote request
   runRequest(pIdx, eIdx);
  }

  /**
   * Function to invoke the execution of a remote function in a remote HiCR instance
   */
  __USED__ inline void invoke(HiCR::Instance* instance, const processingUnitIndex_t pIdx, const executionUnitIndex_t eIdx) override
  {
   // Getting up-casted pointer for the execution unit
   auto MPIInstance = dynamic_cast<HiCR::backend::mpi::Instance *>(instance);

   // Checking whether the execution unit passed is compatible with this backend
   if (MPIInstance == NULL) HICR_THROW_LOGIC("The passed instance type is not supported by this backend (MPI)\n");

   // Getting rank Id for the passed instance
   const auto dest = MPIInstance->getRank();

   // Sending request
   MPI_Send(&eIdx, 1, MPI_UNSIGNED_LONG, dest, _HICR_MPI_COORDINATION_TAG, _comm);
   MPI_Send(&pIdx, 1, MPI_UNSIGNED_LONG, dest, _HICR_MPI_COORDINATION_TAG, _comm);
  }

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
};

} // namespace mpi
} // namespace backend
} // namespace HiCR
