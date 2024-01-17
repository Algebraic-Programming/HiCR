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

#include <map>
#include <memory>
#include <unordered_set>
#include <utility>
#include <functional>
#include <hicr/L0/executionUnit.hpp>
#include <hicr/L0/instance.hpp>
#include <hicr/L0/localMemorySlot.hpp>
#include <hicr/L0/memorySpace.hpp>
#include <hicr/L0/processingUnit.hpp>
#include <hicr/L0/topology.hpp>
#include <hicr/L1/memoryManager.hpp>
#include <hicr/L1/communicationManager.hpp>
#include <hicr/L1/computeManager.hpp>

/**
 * This value is assigned to the default processing unit if no id is provided
 */
#define _HICR_DEFAULT_PROCESSING_UNIT_ID_ 0xF0F0F0F0ul

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
   * Type definition for a listenable unit. That is, the pair of execution unit and the processing unit in charge of executing it
   */
  typedef std::pair<executionUnitIndex_t, processingUnitIndex_t> RPCTarget_t;

  /**
   * Type definition for an index for a listenable unit.
   */
  typedef int RPCTargetIndex_t;

  /**
   * Type definition for an unsorted set of unique pointers to the detected instances
   */
  typedef std::unordered_set<std::shared_ptr<HiCR::L0::Instance>> instanceList_t;

  void finalize() { _processingUnitMap.clear(); }

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
  __USED__ inline const instanceList_t &getInstances() const { return _instances; }

  /**
   * Function to retrieve the currently executing instance
   * \return A pointer to the local HiCR instance (in other words, the one running this function)
   */
  __USED__ inline std::shared_ptr<HiCR::L0::Instance> getCurrentInstance() const { return _currentInstance; }

  /**
   * Function to create a new HiCR instance
   * \param[in] requestedTopology The HiCR topology to try to obtain in the new instance
   * \return A pointer to the newly created instance (if successful), a null pointer otherwise.
   */
  __USED__ inline std::shared_ptr<HiCR::L0::Instance> createInstance(const HiCR::L0::Topology &requestedTopology = HiCR::L0::Topology())
  {
    // Requesting the creating of the instance to the specific backend
    auto newInstance = createInstanceImpl(requestedTopology);

    // If successul, adding the instance to the internal list
    if (newInstance != nullptr) _instances.insert(newInstance);

    // Returning value for immediate use
    return newInstance;
  }

  /**
   * Function to retrieve the internal memory manager for this instance manager
   * \return A pointer to the memory manager used to instantiate this instance manager
   */
  __USED__ inline std::shared_ptr<HiCR::L1::MemoryManager> getMemoryManager() const { return _memoryManager; }

  /**
   * Function to retrieve the internal communication manager for this instance manager
   * \return A pointer to the communication manager used to instantiate this instance manager
   */
  __USED__ inline std::shared_ptr<HiCR::L1::CommunicationManager> getCommunicationManager() const { return _communicationManager; }

  /**
   * Function to retrieve the internal compute manager for this instance manager
   * \return A pointer to the compute manager used to instantiate this instance manager
   */
  __USED__ inline std::shared_ptr<HiCR::L1::ComputeManager> getComputeManager() const { return _computeManager; }

  /**
   * Function to retrieve the internal memory space used for creating buffers in this instance manager
   * \return The internal memory space
   */
  __USED__ inline std::shared_ptr<HiCR::L0::MemorySpace> getBufferMemorySpace() const { return _bufferMemorySpace; }

  /**
   * Function to add a new execution unit, assigned to a unique identifier
   * \param[in] index Indicates the index to assign to the added execution unit
   * \param[in] executionUnit The execution unit to add
   */
  __USED__ inline void addExecutionUnit(std::shared_ptr<HiCR::L0::ExecutionUnit> executionUnit, const executionUnitIndex_t index) { _executionUnitMap[index] = executionUnit; }

  /**
   * Function to add a new processing unit, assigned to a unique identifier
   * \param[in] index Indicates the index to assign to the added processing unit
   * \param[in] processingUnit The processing unit to add
   */
  __USED__ inline void addProcessingUnit(std::unique_ptr<HiCR::L0::ProcessingUnit> processingUnit, const processingUnitIndex_t index = _HICR_DEFAULT_PROCESSING_UNIT_ID_) { _processingUnitMap[index] = std::move(processingUnit); }

  /**
   * Function to add an RPC target with a name, and the combination of a execution unit and the processing unit that is in charge of executing it
   * \param[in] RPCName Name of the RPC to add
   * \param[in] eIndex Indicates the index of the execution unit to run when this RPC target is triggered
   * \param[in] pIndex Indicates the processing unit to use for running the specified execution unit
   */
  __USED__ inline void addRPCTarget(const std::string &RPCName, const executionUnitIndex_t eIndex, const processingUnitIndex_t pIndex = _HICR_DEFAULT_PROCESSING_UNIT_ID_)
  {
    // Obtaining hash from the RPC name
    const auto nameHash = getHashFromString(RPCName);

    // Inserting the new entry
    _RPCTargetMap[nameHash] = RPCTarget_t({eIndex, pIndex});
  }

  /**
   * Function to put the current instance to listen for incoming RPCs
   */
  __USED__ inline void listen()
  {
    // Calling the backend-specific implementation of the listen function
    listenImpl();
  }

  /**
   * Function to trigger the execution of a remote function in a remote HiCR instance
   * \param[in] RPCName The name of the RPC to run
   * \param[in] instance Instance on which to run the RPC
   */
  virtual void launchRPC(HiCR::L0::Instance &instance, const std::string &RPCName) const = 0;

  /**
   * Function to submit a return value for the currently running RPC
   * \param[in] value The memory slot containing the RPC's return value
   */
  __USED__ inline void submitReturnValue(std::shared_ptr<HiCR::L0::LocalMemorySlot> value) const
  {
    // Calling backend-specific implementation of this function
    submitReturnValueImpl(value);
  }

  /**
   * Function to get a return value from a remote instance that ran an RPC
   * \param[in] instance Instance from which to read the return value. An RPC request should be sent to that instance before calling this function.
   * \return A pointer to a newly allocated local memory slot containing the return value
   */
  __USED__ inline std::shared_ptr<HiCR::L0::LocalMemorySlot> getReturnValue(HiCR::L0::Instance &instance) const
  {
    // Calling backend-specific implementation of this function
    return getReturnValueImpl(instance);
  }

  /**
   * Function to set the buffer memory space to use for allocations when receiving RPC or return values.
   *
   * Must be set before starting to listen for incoming messages
   *
   * @param[in] bufferMemorySpace The memory space in which to allocate the buffers
   */
  void setBufferMemorySpace(const std::shared_ptr<HiCR::L0::MemorySpace> bufferMemorySpace) { _bufferMemorySpace = bufferMemorySpace; }

  protected:

  /**
   * Generates a 64-bit hash value from a given string. Useful for compressing the name of RPCs
   *
   * @param[in] name A string (e.g., the name of an RPC to compress)
   * @return The 64-bit hashed value of the name provided
   */
  static uint64_t getHashFromString(const std::string &name) { return std::hash<std::string>()(name); }

  /**
   * Constructor with proper arguments
   * \param[in] memoryManager The memory manager to use for buffer allocations
   * \param[in] communicationManager The communication manager to use for internal data passing
   * \param[in] computeManager The compute manager to use for RPC running
   */
  InstanceManager(std::shared_ptr<HiCR::L1::CommunicationManager> communicationManager,
                  std::shared_ptr<HiCR::L1::ComputeManager> computeManager,
                  std::shared_ptr<HiCR::L1::MemoryManager> memoryManager) : _communicationManager(communicationManager),
                                                                            _computeManager(computeManager),
                                                                            _memoryManager(memoryManager){};

  /**
   * Internal function used to initiate the execution of the requested RPC
   * \param[in] rpcIdx Index to the RPC to run (hash to save overhead, the name is no longer recoverable)
   */
  __USED__ inline void executeRPC(const RPCTargetIndex_t rpcIdx)
  {
    // Getting RPC target from the index
    if (_RPCTargetMap.contains(rpcIdx) == false) HICR_THROW_RUNTIME("Attempting to run an RPC target (Hash: %lu) that was not defined in this instance (0x%lX).\n", rpcIdx, this);
    auto &l = _RPCTargetMap[rpcIdx];

    // Getting execute and processing unit indexes
    const auto eIdx = l.first;
    const auto pIdx = l.second;

    // Checks that the processing and execution units have been registered
    if (_processingUnitMap.contains(pIdx) == false) HICR_THROW_RUNTIME("Attempting to run an processing unit (%lu) that was not defined in this instance (0x%lX).\n", pIdx, this);
    if (_executionUnitMap.contains(eIdx) == false) HICR_THROW_RUNTIME("Attempting to run an execution unit (%lu) that was not defined in this instance (0x%lX).\n", eIdx, this);

    // Getting units
    auto &p = *_processingUnitMap[pIdx];
    auto &e = _executionUnitMap[eIdx];

    // Creating execution state
    auto s = _computeManager->createExecutionState(e);

    // Initializing processing unit
    p.initialize();

    // Running execution state
    p.start(std::move(s));

    // Waiting for processing unit to finish
    p.await();
  }

  /**
   * Backend-specific implementation of the createInstance function
   * \param[in] requestedTopology The HiCR topology to try to obtain in the new instance
   * \return A pointer to the newly created instance (if successful), a null pointer otherwise.
   */
  virtual std::shared_ptr<HiCR::L0::Instance> createInstanceImpl(const HiCR::L0::Topology &requestedTopology) = 0;

  /**
   * Backend-specific implementation of the getReturnValue function
   * \param[in] instance Instance from which to read the return value. An RPC request should be sent to that instance before calling this function.
   * \return A pointer to a newly allocated local memory slot containing the return value
   */
  virtual std::shared_ptr<HiCR::L0::LocalMemorySlot> getReturnValueImpl(HiCR::L0::Instance &instance) const = 0;

  /**
   * Backend-specific implementation of the submitReturnValue
   * \param[in] value The memory slot containing the RPC's return value
   */
  virtual void submitReturnValueImpl(std::shared_ptr<HiCR::L0::LocalMemorySlot> value) const = 0;

  /**
   * Backend-specific implementation of the listen function
   */
  virtual void listenImpl() = 0;

  /**
   * Communication manager for exchanging information among HiCR instances
   */
  const std::shared_ptr<HiCR::L1::CommunicationManager> _communicationManager;

  /**
   * Compute manager for running incoming RPCs
   */
  const std::shared_ptr<HiCR::L1::ComputeManager> _computeManager;

  /**
   * Memory manager for allocating internal buffers
   */
  const std::shared_ptr<HiCR::L1::MemoryManager> _memoryManager;

  /**
   * Memory space to store the information bufer into
   */
  std::shared_ptr<HiCR::L0::MemorySpace> _bufferMemorySpace;

  /**
   * Collection of instances
   */
  instanceList_t _instances;

  /**
   * Pointer to current instance
   */
  std::shared_ptr<HiCR::L0::Instance> _currentInstance;

  private:

  /**
   * Map of assigned processing units in charge of executing a execution units
   */
  std::map<processingUnitIndex_t, std::unique_ptr<HiCR::L0::ProcessingUnit>> _processingUnitMap;

  /**
   * Map of execution units, representing potential RPC requests
   */
  std::map<executionUnitIndex_t, std::shared_ptr<HiCR::L0::ExecutionUnit>> _executionUnitMap;

  /**
   * Map of RPC targets units
   */
  std::map<RPCTargetIndex_t, RPCTarget_t> _RPCTargetMap;
};

} // namespace L1

} // namespace HiCR
