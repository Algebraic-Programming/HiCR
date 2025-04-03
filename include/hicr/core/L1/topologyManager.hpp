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
 * @file topologyManager.hpp
 * @brief Provides a definition for the abstract device manager class
 * @author S. M. Martin
 * @date 18/12/2023
 */

#pragma once

#include <nlohmann_json/json.hpp>
#include <memory>
#include "../L0/device.hpp"
#include "../L0/topology.hpp"
#include "memoryManager.hpp"

namespace HiCR::L1
{

/**
 * Encapsulates a HiCR Backend Device Manager.
 *
 * The purpose of this class is to discover the computing topology for a given device type.
 * E.g., if this is a backend for an NPU device and the system contains 8 such devices, it will discover an
 * array of Device type of size 8.
 *
 * Backends need to fulfill the abstract virtual functions described here, so that HiCR can detect hardware devices
 */
class TopologyManager
{
  public:

  /**
   * Default destructor
   */
  virtual ~TopologyManager() = default;

  /**
   * This function prompts the backend to perform the necessary steps to discover the system topology
   *
   * In case of change in resource availability during runtime, users need to re-run this function to be able to see the changes.
   *
   * @return The detected topology, as detected by the specific b ackend
   */
  virtual HiCR::L0::Topology queryTopology() = 0;

  /**
   * This function deserializes a JSON-encoded topology into a topology class with its constituent devices, as recognized by the called backend, and returns it
   *
   * If the backend does not recognize a device in the encoded topology, it will not add it to the topology
   *
   * @param[in] topology The JSON-encoded topology to deserialize
   * @return The deserialized topology containing only devices recognized by the backend
   */
  [[nodiscard]] virtual HiCR::L0::Topology _deserializeTopology(const nlohmann::json &topology) const = 0;

  protected:

  /**
   * Protected default constructor  used to build a new instance of this topology manager based on serialized information
   *
   * \note The instance created by this constructor should only be used to print/query the topology. It cannot be used to operate (memcpy, compute, etc).
   */
  TopologyManager() = default;
};

} // namespace HiCR::L1
