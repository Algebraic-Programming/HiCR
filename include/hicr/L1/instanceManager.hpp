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

#include <hicr/L0/executionUnit.hpp>
#include <hicr/L0/instance.hpp>
#include <hicr/L0/localMemorySlot.hpp>
#include <hicr/L0/memorySpace.hpp>
#include <hicr/L0/processingUnit.hpp>
#include <hicr/L1/memoryManager.hpp>
#include <hicr/L1/communicationManager.hpp>
#include <hicr/L1/computeManager.hpp>
#include <map>
#include <memory>
#include <set>
#include <utility>

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
   * Type definition for an index to indicate the execution of a specific execution unit
   */
  typedef uint64_t executionUnitIndex_t;

  /**
   * Type definition for an index to indicate the use of a specific processing unit in charge of executing a execution units
   */
  typedef uint64_t processingUnitIndex_t;

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
   * Function to retrieve the internal communication manager for this instance manager
   * \return A pointer to the communication manager used to instantiate this instance manager
   */
  __USED__ inline HiCR::L1::CommunicationManager *getCommunicationManager() const { return _communicationManager; }

  /**
   * Function to retrieve the internal compute manager for this instance manager
   * \return A pointer to the compute manager used to instantiate this instance manager
   */
  __USED__ inline HiCR::L1::ComputeManager *getComputeManager() const { return _computeManager; }

   /**
   * Function to retrieve the internal memory space used for creating buffers in this instance manager
   * \return The internal memory space
   */
  __USED__ inline HiCR::L0::MemorySpace* getBufferMemorySpace() const { return _bufferMemorySpace; }

  /**
   * Function to add a new execution unit, assigned to a unique identifier
   * \param[in] index Indicates the index to assign to the added execution unit
   * \param[in] executionUnit The execution unit to add
   */
  __USED__ inline void addExecutionUnit(const executionUnitIndex_t index, HiCR::L0::ExecutionUnit *executionUnit) { _executionUnitMap[index] = executionUnit; }

  /**
   * Function to add a new processing unit, assigned to a unique identifier
   * \param[in] index Indicates the index to assign to the added processing unit
   * \param[in] processingUnit The processing unit to add
   */
  __USED__ inline void addProcessingUnit(const processingUnitIndex_t index, std::unique_ptr<HiCR::L0::ProcessingUnit> processingUnit) { _processingUnitMap[index] = std::move(processingUnit); }

  /**
   * Function to put the current instance to listen for incoming requests
   */
  __USED__ inline void listen()
  {
    // Calling the backend-specific implementation of the listen function
    listenImpl();
  }

  /**
   * Function to trigger the execution of a remote function in a remote HiCR instance
   * \param[in] pIdx Index to the processing unit to use
   * \param[in] eIdx Index to the execution unit to run
   * \param[in] instance Instance on which to run the RPC
   */
  virtual void execute(HiCR::L0::Instance *instance, const processingUnitIndex_t pIdx, const executionUnitIndex_t eIdx) const = 0;

  /**
   * Function to submit a return value for the currently running RPC
   * \param[in] value The memory slot containing the RPC's return value
   */
  __USED__ inline void submitReturnValue(HiCR::L0::LocalMemorySlot *value) const
  {
    // Calling backend-specific implementation of this function
    submitReturnValueImpl(value);
  }

  /**
   * Function to get a return value from a remote instance that ran an RPC
   * \param[in] instance Instance from which to read the return value. An RPC request should be sent to that instance before calling this function.
   * \return A pointer to a newly allocated local memory slot containing the return value
   */
  __USED__ inline HiCR::L0::LocalMemorySlot *getReturnValue(HiCR::L0::Instance *instance) const
  {
    // Calling backend-specific implementation of this function
    return getReturnValueImpl(instance);
  }

  protected:

  /**
   * Constructor with proper arguments
   * \param[in] memoryManager The memory manager to use for buffer allocations
   * \param[in] communicationManager The communication manager to use for internal data passing
   * \param[in] computeManager The compute manager to use for RPC running
   * \param[in] bufferMemorySpace The memory space from which to allocate data buffers
   */
  InstanceManager(HiCR::L1::CommunicationManager *const communicationManager,
                  HiCR::L1::ComputeManager *const computeManager,
                  HiCR::L1::MemoryManager *const memoryManager,
                  HiCR::L0::MemorySpace *const bufferMemorySpace) :
                  _communicationManager(communicationManager),
                  _computeManager(computeManager),
                  _memoryManager(memoryManager),
                  _bufferMemorySpace(bufferMemorySpace)
  { };

  /**
   * Internal function used to initiate the execution of the requested RPC  bt running executionUnit using the indicated procesing unit
   * \param[in] pIdx Index to the processing unit to use
   * \param[in] eIdx Index to the execution unit to run
   */
  __USED__ inline void runRequest(const processingUnitIndex_t pIdx, const executionUnitIndex_t eIdx)
  {
    // Checks that the processing and execution units have been registered
    if (_processingUnitMap.contains(pIdx) == false) HICR_THROW_RUNTIME("Attempting to run an processing unit (%lu) that was not defined in this instance (0x%lX).\n", pIdx, this);
    if (_executionUnitMap.contains(eIdx) == false) HICR_THROW_RUNTIME("Attempting to run an execution unit (%lu) that was not defined in this instance (0x%lX).\n", eIdx, this);

    // Getting units
    auto &p = _processingUnitMap[pIdx];
    auto &e = _executionUnitMap[eIdx];

    // Creating execution state
    auto s = _computeManager->createExecutionState(e);

    // Running execution state
    p->start(std::move(s));
  }

  /**
   * Backend-specific implementation of the getReturnValue function
   * \param[in] instance Instance from which to read the return value. An RPC request should be sent to that instance before calling this function.
   * \return A pointer to a newly allocated local memory slot containing the return value
   */
  virtual HiCR::L0::LocalMemorySlot *getReturnValueImpl(HiCR::L0::Instance *instance) const = 0;

  /**
   * Backend-specific implementation of the submitReturnValue
   * \param[in] value The memory slot containing the RPC's return value
   */
  virtual void submitReturnValueImpl(HiCR::L0::LocalMemorySlot *value) const = 0;

  /**
   * Backend-specific implementation of the listen function
   */
  virtual void listenImpl() = 0;

  /**
  * Communication manager for exchanging information among HiCR instances
  */
  HiCR::L1::CommunicationManager *const _communicationManager;

  /**
  * Compute manager for running incoming RPCs
  */
  HiCR::L1::ComputeManager *const _computeManager;

  /**
   * Memory manager for allocating internal buffers
   */
  HiCR::L1::MemoryManager *const _memoryManager;

    /**
   * Memory space to store the information bufer into
   */
  HiCR::L0::MemorySpace *const _bufferMemorySpace;

  /**
   * Collection of instances
   */
  std::set<HiCR::L0::Instance *> _instances;

  /**
   * Pointer to current instance
   */
  HiCR::L0::Instance *_currentInstance = NULL;

  private:

  /**
   * Map of assigned processing units in charge of executing a execution units
   */
  std::map<processingUnitIndex_t, std::unique_ptr<HiCR::L0::ProcessingUnit>> _processingUnitMap;

  /**
   * Map of execution units, representing potential RPC requests
   */
  std::map<executionUnitIndex_t, HiCR::L0::ExecutionUnit *> _executionUnitMap;
};

} // namespace L1

} // namespace HiCR
