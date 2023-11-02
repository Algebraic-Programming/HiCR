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

    // If this is not the root rank, enter waiting state here
    if (_rank != _HICR_MPI_ROOT_RANK) MPI_Recv(NULL, 0, MPI_BYTE, _HICR_MPI_ROOT_RANK, _HICR_MPI_COORDINATION_TAG, _comm, MPI_STATUS_IGNORE);
  }

  ~InstanceManager() = default;

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
