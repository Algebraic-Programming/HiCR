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

#include <memory>
#include <pthread.h>

#include <hicr/core/instanceManager.hpp>

#include "core.hpp"
#include "instance.hpp"

namespace HiCR::backend::pthreads
{

/**
 * Implementation of HiCR InstanceManager class. It creates new HiCR Instance using pthreads
*/
class InstanceManager final : public HiCR::InstanceManager
{
  public:

  /**
   * Constructor
   * 
   * \param[in] core pthread core
  */
  InstanceManager(Core &core)
    : HiCR::InstanceManager(),
      _core(core)
  {
    // Create and set current instance in the base class
    setCurrentInstance(_core.addInstance(pthread_self()));

    // Get root instance id
    _rootInstanceId = _core.getRootInstanceId();
  }

  ~InstanceManager() override = default;

  /**
   * Detect all the running instances
   * 
   * \note this call is collective that needs to be called by all the instances registered in the core
  */
  void detectInstances()
  {
    // Wait for all the threads to add their own instance
    _core.fence();

    // Add all the instances to the base class
    for (auto &i : _core.getInstances()) { addInstance(i); }
  }

  /**
   * Create a new instance inside a pthread
   * 
   * \param[in] instanceTemplate instance template used to create the instance
   * 
   * \return a HiCR instance
   * 
   * \note not supported
  */
  std::shared_ptr<HiCR::Instance> createInstanceImpl(const HiCR::InstanceTemplate instanceTemplate) override { HICR_THROW_RUNTIME("This backend does not support this operation"); }

  /**
   * Add an instance.
   * 
   * \param[in] instanceId Id of the instance
   * 
   * \return a HiCR instance
  */
  std::shared_ptr<HiCR::Instance> addInstanceImpl(Instance::instanceId_t instanceId) override
  {
    // Get the instance from the core. Someone else already created it
    auto instance = _core.getInstance(instanceId);
    return instance;
  }

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
  void finalize() override {}

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

  private:

  /**
   * Reference to pthread core
  */
  Core &_core;

  /**
   * Id of the HiCR root Instance
  */
  HiCR::Instance::instanceId_t _rootInstanceId;
};
} // namespace HiCR::backend::pthreads