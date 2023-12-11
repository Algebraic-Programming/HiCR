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
  InstanceManager(HiCR::L1::MemoryManager *const memoryManager) : HiCR::L1::InstanceManager(memoryManager)
  {
    // Getting up-casted pointer for the MPI memory manager
    auto mm = dynamic_cast<mpi::L1::MemoryManager *const>(_memoryManager);

    // Checking whether the execution unit passed is compatible with this backend
    if (mm == NULL) HICR_THROW_LOGIC("The passed memory manager is not supported by this instance manager\n");

    // In MPI, the initial set of processes represents all the currently available instances of HiCR
    for (int i = 0; i < mm->getSize(); i++)
    {
      // Creating new MPI-based HiCR instance
      auto instance = new HiCR::backend::mpi::Instance(i, mm);

      // If this is the current rank, set it as current instance
      if (i == mm->getRank()) _currentInstance = instance;

      // Adding instance to the collection
      _instances.insert(instance);
    }

    // Getting MPI-specific current instance pointer
    auto currentInstanceMPIPtr = (mpi::Instance *)_currentInstance;

    // Exchanging global slots for instance state values
    mm->exchangeGlobalMemorySlots(_HICR_MPI_INSTANCE_MANAGER_TAG, {std::make_pair(currentInstanceMPIPtr->getRank(), currentInstanceMPIPtr->getStateLocalMemorySlot())});

    // Getting globally exchanged slots
    for (auto instance : _instances)
    {
      // Getting MPI-specific instance pointer
      auto instanceMPIPtr = (mpi::Instance *)instance;

      // Getting global slot pointer
      auto globalSlot = mm->getGlobalMemorySlot(_HICR_MPI_INSTANCE_MANAGER_TAG, instanceMPIPtr->getRank());

      // Assigning it to the corresponding instance
      instanceMPIPtr->setStateGlobalMemorySlot(globalSlot);
    }
  }

  ~InstanceManager() = default;
};

} // namespace L1

} // namespace mpi

} // namespace backend

} // namespace HiCR
