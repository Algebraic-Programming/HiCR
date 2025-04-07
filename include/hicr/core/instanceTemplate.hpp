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
 * @file instanceTemplate.hpp
 * @brief Provides a definition for the HiCR Instance Template class.
 * @author L. Terracciano S. M. Martin
 * @date 28/03/2025
 */

#pragma once

#include <cstdint>
#include <hicr/core/definitions.hpp>
#include <hicr/core/instance.hpp>
#include <hicr/core/topology.hpp>
#include <hicr/core/instanceManager.hpp>

namespace HiCR
{

/**
 * Forward declaration of HiCR Instance Manager -- avoid circular dependency
*/
class InstanceManager;


/**
 * Defines the instance template class, which represents a blueprint to create new instances.
 */
class InstanceTemplate
{
  friend class HiCR::InstanceManager;

  public:

  /**
   * Constructor for the base instance template class.
   *
   * \param[in] topology HiCR topology used to construct instances
   *
   */
  InstanceTemplate(const HiCR::Topology &topology)
    : _topology(topology){};
  InstanceTemplate() = delete;

  /**
   * Default destructor
   */
  virtual ~InstanceTemplate() = default;

  /**
   * Getter for the topology
   * 
   * \return topology
  */
  const HiCR::Topology &getTopology() const { return _topology; }

  private:

  /**
   * Instance Identifier
   */
  const HiCR::Topology _topology;
};

} // namespace HiCR
