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
 * @brief This file implements the instance manager class for the  HWLoc-based backend (single instance)
 * @author S. M. Martin
 * @date 4/4/2024
 */

#pragma once

#include <memory>
#include <hicr/core/definitions.hpp>
#include <hicr/backends/hwloc/instance.hpp>
#include <hicr/core/instanceManager.hpp>

namespace HiCR::backend::hwloc
{

/**
 * Implementation of the HiCR hwloc Instance Manager
 */
class InstanceManager final : public HiCR::InstanceManager
{
  public:

  /**
   * Constructor for the hwloc instance manager
   */
  InstanceManager()
    : HiCR::InstanceManager()
  {
    // A single instance (the currently running) is created and is deemed as the root
    auto currentInstance = std::make_shared<HiCR::backend::hwloc::Instance>();
    setCurrentInstance(currentInstance);
    addInstance(currentInstance);
  }

  ~InstanceManager() override = default;

  __INLINE__ void finalize() override {}

  __INLINE__ void abort(int errorCode) override { std::abort(); }

  /**
   * This function represents the default intializer for this backend
   *
   * @param[in] argc A pointer to the argc value, as passed to main() necessary for new HiCR instances to count with argument information upon creation.
   * @param[in] argv A pointer to the argv value, as passed to main() necessary for new HiCR instances to count with argument information upon creation.
   * @return A unique pointer to the newly instantiated backend class
   */
  __INLINE__ static std::unique_ptr<HiCR::InstanceManager> createDefault(int *argc, char ***argv)
  {
    // Creating instance manager
    return std::make_unique<HiCR::backend::hwloc::InstanceManager>();
  }

  [[nodiscard]] __INLINE__ HiCR::Instance::instanceId_t getRootInstanceId() const override { return 0; }

  protected:

  __INLINE__ std::shared_ptr<HiCR::Instance> createInstanceImpl(const std::shared_ptr<HiCR::InstanceTemplate> &instanceTemplate) override
  {
    HICR_THROW_LOGIC("The Host backend does not currently support the launching of new instances during runtime");
  }

  __INLINE__ std::shared_ptr<HiCR::Instance> addInstanceImpl(HiCR::Instance::instanceId_t instanceId) override
  {
    HICR_THROW_LOGIC("The Host backend does not currently support the detection of new instances during runtime");
  }

  private:

  /**
   * The return value buffer is stored locally 
  */
  void *_returnValueBuffer{};
};

} // namespace HiCR::backend::hwloc
