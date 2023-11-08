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
#include <hicr/backends/mpi/memoryManager.hpp>

namespace HiCR
{

namespace backend
{

namespace mpi
{

#define _HICR_MPI_INSTANCE_MANAGER_TAG 4096

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
  InstanceManager(MPI_Comm comm = MPI_COMM_WORLD) : _memoryManager(new mpi::MemoryManager(comm))
  {
    // In MPI, the initial set of processes represents all the currently available instances of HiCR
    for (int i = 0; i < _memoryManager->getSize(); i++)
    {
      // Creating new MPI-based HiCR instance
      auto instance = new HiCR::backend::mpi::Instance(i, comm, _memoryManager);

      // If this is the current rank, set it as current instance
      if (i == _memoryManager->getRank())  _currentInstance = instance;

     // Adding instance to the collection
     _instances.insert(instance);
    }

   // Getting MPI-specific current instance pointer
   auto currentInstanceMPIPtr = (mpi::Instance*) _currentInstance;

   // Exchanging global slots for instance state values
   _memoryManager->exchangeGlobalMemorySlots(_HICR_MPI_INSTANCE_MANAGER_TAG, { std::make_pair(currentInstanceMPIPtr->getRank(), currentInstanceMPIPtr->getStateLocalMemorySlot()) });

   // Getting globally exchanged slots
   for (auto instance : _instances)
   {
    // Getting MPI-specific instance pointer
    auto instanceMPIPtr = (mpi::Instance*) instance;

    // Getting global slot pointer
    auto globalSlot = _memoryManager->getGlobalMemorySlot(_HICR_MPI_INSTANCE_MANAGER_TAG, instanceMPIPtr->getRank());

    // Assigning it to the corresponding instance
    instanceMPIPtr->setStateGlobalMemorySlot(globalSlot);
   }
  }

  ~InstanceManager() = default;

  __USED__ inline bool isCoordinatorInstance() override
  {
   // For the MPI backend, the coordinator instance will be set to process with rank _HICR_MPI_INSTANCE_ROOT_RANK
   if (_memoryManager->getRank() == _HICR_MPI_INSTANCE_ROOT_RANK) return true;
   return false;
  }

  private:

  /**
   * Memory manager object for exchanging information among HiCR instances
   */
  mpi::MemoryManager* const _memoryManager;
};

} // namespace mpi
} // namespace backend
} // namespace HiCR
