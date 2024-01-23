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
   * Type definition for an index for a listenable unit.
   */
  typedef uint64_t RPCTargetIndex_t;

  /**
   * Type definition for a function that can be executed as RPC
   */
  typedef std::function<void()> RPCFunction_t;

  /**
   * Type definition for an unsorted set of unique pointers to the detected instances
   */
  typedef std::unordered_set<std::shared_ptr<HiCR::L0::Instance>> instanceList_t;

  /**
   * Default constructor is deleted, this class requires the passing of a memory manager
   */
  InstanceManager() = default;

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
   * \param[in] argc Argc to pass to the newly created instance
   * \param[in] argv Argv to pass to the newly created instance
   * \param[in] requestedTopology The HiCR topology to try to obtain in the new instance
   * \return A pointer to the newly created instance (if successful), a null pointer otherwise.
   */
  __USED__ inline std::shared_ptr<HiCR::L0::Instance> createInstance(const HiCR::L0::Topology &requestedTopology = HiCR::L0::Topology(), int argc = 0, char *argv[] = nullptr)
  {
    // Requesting the creating of the instance to the specific backend
    auto newInstance = createInstanceImpl(requestedTopology, argc, argv);

    // If successul, adding the instance to the internal list
    _instances.insert(newInstance);

    // Returning value for immediate use
    return newInstance;
  }

  /**
   * Function to add an RPC target with a name, and the combination of a execution unit and the processing unit that is in charge of executing it
   * \param[in] RPCName Name of the RPC to add
   * \param[in] fc Indicates function to run when this RPC is triggered
   */
  __USED__ inline void addRPCTarget(const std::string &RPCName, const RPCFunction_t fc)
  {
    // Obtaining hash from the RPC name
    const auto nameHash = getHashFromString(RPCName);

    // Inserting the new entry
    _RPCTargetMap[nameHash] = fc;
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
   * \param[in] pointer Pointer to the start of the data buffer to send
   * \param[in] size Size of the data buffer to send
   */
  __USED__ inline void submitReturnValue(void *pointer, const size_t size) const
  {
    // Calling backend-specific implementation of this function
    submitReturnValueImpl(pointer, size);
  }

  /**
   * Function to get a return value from a remote instance that ran an RPC
   * \param[in] instance Instance from which to read the return value. An RPC request should be sent to that instance before calling this function.
   * \return A pointer to a newly allocated local memory slot containing the return value
   */
  __USED__ inline void *getReturnValue(HiCR::L0::Instance &instance) const
  {
    // Calling backend-specific implementation of this function
    return getReturnValueImpl(instance);
  }

  /**
   * This function calls the internal implementation of the finalization procedure for the given instance manager
   */
  virtual void finalize() = 0;

  protected:

  /**
   * Generates a 64-bit hash value from a given string. Useful for compressing the name of RPCs
   *
   * @param[in] name A string (e.g., the name of an RPC to compress)
   * @return The 64-bit hashed value of the name provided
   */
  static uint64_t getHashFromString(const std::string &name) { return std::hash<std::string>()(name); }

  /**
   * Internal function used to initiate the execution of the requested RPC
   * \param[in] rpcIdx Index to the RPC to run (hash to save overhead, the name is no longer recoverable)
   */
  __USED__ inline void executeRPC(const RPCTargetIndex_t rpcIdx)
  {
    // Getting RPC target from the index
    if (_RPCTargetMap.contains(rpcIdx) == false) HICR_THROW_RUNTIME("Attempting to run an RPC target (Hash: %lu) that was not defined in this instance (0x%lX).\n", rpcIdx, this);
    auto &fc = _RPCTargetMap[rpcIdx];

    // Running RPC function
    fc();
  }

  /**
   * Backend-specific implementation of the createInstance function
   * \param[in] argc Argc to pass to the newly created instance
   * \param[in] argv Argv to pass to the newly created instance
   * \param[in] requestedTopology The HiCR topology to try to obtain in the new instance
   * \return A pointer to the newly created instance (if successful), a null pointer otherwise.
   */
  virtual std::shared_ptr<HiCR::L0::Instance> createInstanceImpl(const HiCR::L0::Topology &requestedTopology, int argc, char *argv[]) = 0;

  /**
   * Backend-specific implementation of the getReturnValue function
   * \param[in] instance Instance from which to read the return value. An RPC request should be sent to that instance before calling this function.
   * \return A pointer to a newly allocated local memory slot containing the return value
   */
  virtual void *getReturnValueImpl(HiCR::L0::Instance &instance) const = 0;

  /**
   * Backend-specific implementation of the submitReturnValue
   * \param[in] pointer Pointer to the start of the data buffer to send
   * \param[in] size Size of the data buffer to send
   */
  virtual void submitReturnValueImpl(const void *pointer, const size_t size) const = 0;

  /**
   * Backend-specific implementation of the listen function
   */
  virtual void listenImpl() = 0;

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
   * Map of executable functions, representing potential RPC requests
   */
  std::map<RPCTargetIndex_t, RPCFunction_t> _RPCTargetMap;
};

} // namespace L1

} // namespace HiCR
