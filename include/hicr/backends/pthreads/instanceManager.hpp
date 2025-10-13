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

/**
 * @file instanceManager.hpp
 * @brief This file implements the instance manager class for the Pthreads backend
 * @author L. Terracciano
 * @date 10/10/2025
 */

#pragma once

#include <pthread.h>

#include <hicr/core/instanceManager.hpp>

#include "instance.hpp"

namespace HiCR::backend::pthreads
{

/**
 * Type for the instance entrypoint function
 */
typedef std::function<void(InstanceManager *)> entryPoint_t;

/**
 * Implementation of HiCR InstanceManager class. It creates new HiCR Instance using pthreads
*/
class InstanceManager final : public HiCR::InstanceManager
{
  public:

  /**
   * Constructor
   * 
   * \param[in] rootInstanceId Id of HiCR root Instance
   * \param[in] entrypoint function executed by the Instances when created
  */
  InstanceManager(Instance::instanceId_t rootInstanceId, entryPoint_t entrypoint)
    : HiCR::InstanceManager(),
      _rootInstanceId(rootInstanceId),
      _entrypoint(entrypoint)
  {
    // Create and set current instance in the base class
    setCurrentInstance(std::make_shared<Instance>(pthread_self(), _rootInstanceId));
  }

  ~InstanceManager() override = default;

  /**
   * Create a new instance inside a pthread
   * 
   * \param[in] instanceTemplate instance template used to create the instance
   * 
   * \return a HiCR instance
  */
  std::shared_ptr<HiCR::Instance> createInstanceImpl(const HiCR::InstanceTemplate instanceTemplate) override
  {
    // Storage for the new instance id
    pthread_t newInstanceId;

    // Launch a new pthread executing the entrypoint
    auto status = pthread_create(&newInstanceId, nullptr, launchWrapper, this);
    if (status != 0) { HICR_THROW_RUNTIME("Could not create instance thread. Error: %d", status); }

    // Add to the pool of created pthreads
    _createdThreads.insert(newInstanceId);

    // Create a new HiCR instance
    return std::make_shared<Instance>(newInstanceId, _rootInstanceId);
  }

  /**
   * Add an instance.
   * 
   * \param[in] instanceId Id of the instance
   * 
   * \return a HiCR instance
  */
  std::shared_ptr<HiCR::Instance> addInstanceImpl(Instance::instanceId_t instanceId) override { return std::make_shared<Instance>(instanceId, _rootInstanceId); }

  /**
   * Terminate an instance. Nothing to do other than waiting for the pthread to finish
   * 
   * \param[in] instance instance to terminate
  */
  void terminateInstanceImpl(const std::shared_ptr<HiCR::Instance> instance) override
  {
    // Nothing to do here
  }

  /**
   * Wait for all created threads to finalize
   */
  void finalize() override
  {
    for (auto thread : _createdThreads) { pthread_join(thread, nullptr); }
  }

  /**
   * Abort execution
   * 
   * \param[in] errorCode exit code
  */
  void abort(int errorCode) override { exit(errorCode); }

  /**
   * Getter for root instance id
   * 
   * \return root instance id
  */
  HiCR::Instance::instanceId_t getRootInstanceId() const override { return _rootInstanceId; }

  /**
   * Getter for the entrypoint. Useful if the intention is to 
   * propagate the same entrypoint across instance managers
   * 
   * \return the entrypoint function
  */
  entryPoint_t getEntrypoint() const { return _entrypoint; }

  private:

  /**
   * Wrapper to launch the entrypoint of the new instance
   * 
   * \param[in] parentInstanceManager instance manager of the creator instance
  */
  __INLINE__ static void *launchWrapper(void *parentInstanceManager)
  {
    // Cast to a Pthread InstanceManager
    auto p = static_cast<InstanceManager *>(parentInstanceManager);

    // Run the entrypoint
    p->_entrypoint(p);
    return 0;
  }

  /**
   * Id of the HiCR root Instance
  */
  HiCR::Instance::instanceId_t _rootInstanceId;

  /**
   * Function that each newly created instance runs
   */
  entryPoint_t _entrypoint;

  /**
   * Pool of threads created by the Instance Manager
  */
  std::unordered_set<pthread_t> _createdThreads;
};
} // namespace HiCR::backend::pthreads