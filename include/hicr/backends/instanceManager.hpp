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
#include <hicr/backends/memoryManager.hpp>
#include <hicr/instance.hpp>

namespace HiCR
{

namespace backend
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
   */
  __USED__ inline const std::set<HiCR::Instance*>& getInstances() const { return _instances; }

  /**
   * Function to retrieve the currently executing instance
   */
  __USED__ inline HiCR::Instance* getCurrentInstance() const { return _currentInstance; }

  /**
   * Function to retrieve the internal memory manager for this instance manager
   */
  __USED__ inline HiCR::backend::MemoryManager* getMemoryManager() const { return _memoryManager; }

  /**
   * Collection of instances
   */
  std::set<HiCR::Instance*> _instances;

  /**
   * Pointer to current instance
   */
  HiCR::Instance* _currentInstance = NULL;

  protected:

  /**
   * Constructor with proper arguments
   */
  InstanceManager(HiCR::backend::MemoryManager* const memoryManager) : _memoryManager(memoryManager)
  {
   // Querying memory spaces in the memory manager
   _memoryManager->queryMemorySpaces();
  };

  /**
   * Memory manager object for exchanging information among HiCR instances
   */
  HiCR::backend::MemoryManager* const _memoryManager;

};

} // namespace backend

} // namespace HiCR
