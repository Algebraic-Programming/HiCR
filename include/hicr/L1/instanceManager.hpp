/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file instanceManager.hpp
 * @brief Provides a definition for the abstract instance manager class
 * @author S. M. Martin
 * @date 2/11/2023
 */

#pragma once

#include <set>
#include <hicr/L0/instance.hpp>
#include <hicr/L1/memoryManager.hpp>


namespace HiCR
{

namespace L1
{

/**
 * Encapsulates a HiCR Backend Instance Manager.
 *
 * Backends need to fulfill the abstract virtual functions described here, so that HiCR can detect/create/communicate with other HiCR instances
 *
 */
class InstanceManager
{
  public:

  /**
   * Default constructor is deleted, this class requires the passing of a memory manager
   */
  InstanceManager() = delete;

  /**
   * Default destructor
   */
  virtual ~InstanceManager() = default;

  /**
   * This function prompts the backend to perform the necessary steps to discover and list the currently created (active or not)
   * \return A set of pointers to HiCR instances that refer to both local and remote instances
   */
  __USED__ inline const std::set<HiCR::L0::Instance *> &getInstances() const { return _instances; }

  /**
   * Function to retrieve the currently executing instance
   * \return A pointer to the local HiCR instance (in other words, the one running this function)
   */
  __USED__ inline HiCR::L0::Instance *getCurrentInstance() const { return _currentInstance; }

  /**
   * Function to retrieve the internal memory manager for this instance manager
   * \return A pointer to the memory manager used to instantiate this instance manager
   */
  __USED__ inline HiCR::L1::MemoryManager *getMemoryManager() const { return _memoryManager; }

  /**
   * Collection of instances
   */
  std::set<HiCR::L0::Instance *> _instances;

  /**
   * Pointer to current instance
   */
  HiCR::L0::Instance *_currentInstance = NULL;

  protected:

  /**
   * Constructor with proper arguments
   * \param memoryManager The memory manager to use for exchange of data (state, return values) between instances
   */
  InstanceManager(HiCR::L1::MemoryManager *const memoryManager) : _memoryManager(memoryManager)
  {
    // Querying memory spaces in the memory manager
    _memoryManager->queryMemorySpaces();
  };

  /**
   * Memory manager object for exchanging information among HiCR instances
   */
  HiCR::L1::MemoryManager *const _memoryManager;
};

} // namespace L1

} // namespace HiCR
