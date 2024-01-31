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
#include <hicr/L0/device.hpp>
#include <hicr/L0/topology.hpp>
#include <hicr/L1/memoryManager.hpp>

namespace HiCR
{

namespace L1
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

  protected:

  /**
   * Protected default constructor  used to build a new instance of this topology manager based on serialized information
   *
   * \note The instance created by this constructor should only be used to print/query the topology. It cannot be used to operate (memcpy, compute, etc).
   */
  TopologyManager() = default;
};

} // namespace L1

} // namespace HiCR
