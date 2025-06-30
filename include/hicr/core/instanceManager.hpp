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
#include <hicr/core/executionUnit.hpp>
#include <hicr/core/instance.hpp>
#include <hicr/core/instanceTemplate.hpp>
#include <hicr/core/localMemorySlot.hpp>
#include <hicr/core/memorySpace.hpp>
#include <hicr/core/processingUnit.hpp>
#include <hicr/core/topology.hpp>
#include <hicr/core/memoryManager.hpp>
#include <hicr/core/communicationManager.hpp>
#include <hicr/core/computeManager.hpp>

namespace HiCR
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
  using instanceList_t = std::vector<std::shared_ptr<HiCR::Instance>>;

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
  [[nodiscard]] __INLINE__ instanceList_t &getInstances() { return _instances; }

  /**
   * Function to retrieve the currently executing instance
   * \return A pointer to the local HiCR instance (in other words, the one running this function)
   */
  [[nodiscard]] __INLINE__ std::shared_ptr<HiCR::Instance> getCurrentInstance() const { return _currentInstance; }

  /**
   * Function to create new instance template 
   * \param[in] requestedTopology The HiCR topology to try to obtain in the new instance
   * \return A pointer to the newly created instance template
  */
  __INLINE__ std::shared_ptr<HiCR::InstanceTemplate> createInstanceTemplate(const HiCR::Topology &requestedTopology = HiCR::Topology())
  {
    return std::make_shared<HiCR::InstanceTemplate>(requestedTopology);
  }

  /**
   * Function to create a new HiCR instance
   * \param[in] instanceTemplate The HiCR instance template to try to obtain in the new instance
   * \return A pointer to the newly created instance (if successful), a null pointer otherwise.
   */
  __INLINE__ std::shared_ptr<HiCR::Instance> createInstance(const HiCR::InstanceTemplate instanceTemplate = HiCR::InstanceTemplate())
  {
    // Requesting the creating of the instance to the specific backend
    auto newInstance = createInstanceImpl(instanceTemplate);

    // If successul, adding the instance to the internal list
    if (newInstance != nullptr) _instances.push_back(newInstance);

    // Returning value for immediate use
    return newInstance;
  }

  /**
   * Function to add a new instance to the set of instances tracked by the instance manager.
   * 
   * \param[in] instanceId the id of the instance
  */
  __INLINE__ void addInstance(HiCR::Instance::instanceId_t instanceId)
  {
    // Adding a new instance
    auto instance = addInstanceImpl(instanceId);

    // Adding the instance to the internal list
    _instances.push_back(instance);
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
  [[nodiscard]] virtual HiCR::Instance::instanceId_t getRootInstanceId() const = 0;

  protected:

  /**
   * Backend-specific implementation of the createInstance function
   * \param[in] instanceTemplate The HiCR instance template to try to obtain in the new instance
   * \return A pointer to the newly created instance (if successful), a null pointer otherwise.
   */
  virtual std::shared_ptr<HiCR::Instance> createInstanceImpl(const HiCR::InstanceTemplate instanceTemplate) = 0;

  /**
   * Backend-specific implementation of the addInstance function
   * \param[in] instanceId the id of the instance
   * \return A pointer to the backend-specific instance
  */
  virtual std::shared_ptr<HiCR::Instance> addInstanceImpl(HiCR::Instance::instanceId_t instanceId) = 0;

  protected:

  /**
   * Set the current instance
   * @param[in] instance The instance to set as current instance
   */
  __INLINE__ void setCurrentInstance(const std::shared_ptr<HiCR::Instance> &instance) { _currentInstance = instance; }

  /**
   * Add a new instance to the manager scope
   * @param[in] instance The instance to add
   */
  __INLINE__ void addInstance(const std::shared_ptr<HiCR::Instance> &instance) { _instances.push_back(instance); }

  private:

  /**
   * Collection of instances
   */
  instanceList_t _instances;

  /**
   * Pointer to current instance
   */
  std::shared_ptr<HiCR::Instance> _currentInstance;

  /**
   * Map of executable functions, representing potential RPC requests
   */
  std::map<RPCTargetIndex_t, RPCFunction_t> _RPCTargetMap;
};

} // namespace HiCR
