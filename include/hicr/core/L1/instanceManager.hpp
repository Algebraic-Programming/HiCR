/*
 *   Copyright 2025 Huawei Technologies Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
#include <hicr/core/L0/executionUnit.hpp>
#include <hicr/core/L0/instance.hpp>
#include <hicr/core/L0/instanceTemplate.hpp>
#include <hicr/core/L0/localMemorySlot.hpp>
#include <hicr/core/L0/memorySpace.hpp>
#include <hicr/core/L0/processingUnit.hpp>
#include <hicr/core/L0/topology.hpp>
#include <hicr/core/L1/memoryManager.hpp>
#include <hicr/core/L1/communicationManager.hpp>
#include <hicr/core/L1/computeManager.hpp>

namespace HiCR::L1
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
  using RPCTargetIndex_t = uint64_t;

  /**
   * Type definition for a function that can be executed as RPC
   */
  using RPCFunction_t = std::function<void()>;

  /**
   * Type definition for an unsorted set of unique pointers to the detected instances
   */
  using instanceList_t = std::vector<std::shared_ptr<HiCR::L0::Instance>>;

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
  [[nodiscard]] __INLINE__ const instanceList_t &getInstances() const { return _instances; }

  /**
   * Function to retrieve the currently executing instance
   * \return A pointer to the local HiCR instance (in other words, the one running this function)
   */
  [[nodiscard]] __INLINE__ std::shared_ptr<HiCR::L0::Instance> getCurrentInstance() const { return _currentInstance; }

  /**
   * Function to create new instance template 
   * \param[in] requestedTopology The HiCR topology to try to obtain in the new instance
   * \return A pointer to the newly created instance template
  */
  __INLINE__ std::shared_ptr<HiCR::L0::InstanceTemplate> createInstanceTemplate(const HiCR::L0::Topology &requestedTopology = HiCR::L0::Topology())
  {
    return std::make_shared<HiCR::L0::InstanceTemplate>(requestedTopology);
  }

  /**
   * Function to create a new HiCR instance
   * \param[in] instanceTemplate The HiCR instance template to try to obtain in the new instance
   * \return A pointer to the newly created instance (if successful), a null pointer otherwise.
   */
  __INLINE__ std::shared_ptr<HiCR::L0::Instance> createInstance(const std::shared_ptr<HiCR::L0::InstanceTemplate> &instanceTemplate)
  {
    // Requesting the creating of the instance to the specific backend
    auto newInstance = createInstanceImpl(instanceTemplate);

    // If successul, adding the instance to the internal list
    _instances.push_back(newInstance);

    // Returning value for immediate use
    return newInstance;
  }

  /**
   * Function to add a new instance to the set of instances tracked by the instance manager.
   * 
   * \param[in] instanceId the id of the instance
  */
  __INLINE__ void addInstance(HiCR::L0::Instance::instanceId_t instanceId)
  {
    // Adding a new instance
    auto instance = addInstanceImpl(instanceId);

    // Adding the instance to the internal list
    _instances.push_back(instance);
  }

  /**
   * Function to add an RPC target with a name, and the combination of a execution unit and the processing unit that is in charge of executing it
   * \param[in] RPCName Name of the RPC to add
   * \param[in] fc Indicates function to run when this RPC is triggered
   */
  __INLINE__ void addRPCTarget(const std::string &RPCName, const RPCFunction_t &fc)
  {
    // Obtaining hash from the RPC name
    const auto idx = getRPCTargetIndexFromString(RPCName);

    // Inserting the new entry
    _RPCTargetMap[idx] = fc;
  }

  /**
   * Function to put the current instance to listen for incoming RPCs
   */
  __INLINE__ void listen()
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
  __INLINE__ void submitReturnValue(void *pointer, const size_t size)
  {
    // Calling backend-specific implementation of this function
    submitReturnValueImpl(pointer, size);
  }

  /**
   * Function to get a return value from a remote instance that ran an RPC
   * \param[in] instance Instance from which to read the return value. An RPC request should be sent to that instance before calling this function.
   * \return A pointer to a newly allocated local memory slot containing the return value
   */
  __INLINE__ void *getReturnValue(HiCR::L0::Instance &instance) const
  {
    // Calling backend-specific implementation of this function
    return getReturnValueImpl(instance);
  }

  /**
   * This function calls the internal implementation of the finalization procedure for the given instance manager
   */
  virtual void finalize() = 0;

  /**
   * This function calls the internal implementation of the abort procedure for the given instance manager
   *
   * @param[in] errorCode The error code to publish upon aborting execution
   */
  virtual void abort(int errorCode) = 0;

  /**
   * Returns the id of the backend's root instance id
   * @return The id of the root instance id
   */
  [[nodiscard]] virtual HiCR::L0::Instance::instanceId_t getRootInstanceId() const = 0;

  /**
   * Generates a 64-bit hash value from a given string. Useful for compressing the name of RPCs
   *
   * @param[in] name A string (e.g., the name of an RPC to compress)
   * @return The 64-bit hashed value of the name provided
   */
  static RPCTargetIndex_t getRPCTargetIndexFromString(const std::string &name) { return std::hash<std::string>()(name); }

  /**
   * Internal function used to initiate the execution of the requested RPC
   * \param[in] rpcIdx Index to the RPC to run (hash to save overhead, the name is no longer recoverable)
   */
  __INLINE__ void executeRPC(const RPCTargetIndex_t rpcIdx) const
  {
    // Getting RPC target from the index
    if (_RPCTargetMap.contains(rpcIdx) == false) HICR_THROW_RUNTIME("Attempting to run an RPC target (Hash: %lu) that was not defined in this instance (0x%lX).\n", rpcIdx, this);
    auto &fc = _RPCTargetMap.at(rpcIdx);

    // Running RPC function
    fc();
  }

  protected:

  /**
   * Backend-specific implementation of the createInstance function
   * \param[in] instanceTemplate The HiCR instance template to try to obtain in the new instance
   * \return A pointer to the newly created instance (if successful), a null pointer otherwise.
   */
  virtual std::shared_ptr<HiCR::L0::Instance> createInstanceImpl(const std::shared_ptr<HiCR::L0::InstanceTemplate> &instanceTemplate) = 0;

  /**
   * Backend-specific implementation of the addInstance function
   * \param[in] instanceId the id of the instance
   * \return A pointer to the backend-specific instance
  */
  virtual std::shared_ptr<HiCR::L0::Instance> addInstanceImpl(HiCR::L0::Instance::instanceId_t instanceId) = 0;

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
  virtual void submitReturnValueImpl(const void *pointer, const size_t size) = 0;

  /**
   * Backend-specific implementation of the listen function
   */
  virtual void listenImpl() = 0;

  protected:

  /**
   * Set the current instance
   * @param[in] instance The instance to set as current instance
   */
  __INLINE__ void setCurrentInstance(const std::shared_ptr<HiCR::L0::Instance> &instance) { _currentInstance = instance; }

  /**
   * Add a new instance to the manager scope
   * @param[in] instance The instance to add
   */
  __INLINE__ void addInstance(const std::shared_ptr<HiCR::L0::Instance> &instance) { _instances.push_back(instance); }

  private:

  /**
   * Collection of instances
   */
  instanceList_t _instances;

  /**
   * Pointer to current instance
   */
  std::shared_ptr<HiCR::L0::Instance> _currentInstance;

  /**
   * Map of executable functions, representing potential RPC requests
   */
  std::map<RPCTargetIndex_t, RPCFunction_t> _RPCTargetMap;
};

} // namespace HiCR::L1
