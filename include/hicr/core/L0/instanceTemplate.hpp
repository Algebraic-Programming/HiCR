/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
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
#include <hicr/core/L0/instance.hpp>
#include <hicr/core/L0/topology.hpp>
#include <hicr/core/L1/instanceManager.hpp>

namespace HiCR::L1
{
class InstanceManager;
}

namespace HiCR::L0
{

/**
 * Defines the instance template class, which represents a blueprint to create new instances.
 */
class InstanceTemplate
{
  friend class HiCR::L1::InstanceManager;

  public:

  /**
   * Constructor for the base instance template class.
   *
   * \param[in] topology HiCR topology used to construct instances
   *
   */
  InstanceTemplate(const HiCR::L0::Topology &topology)
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
  const HiCR::L0::Topology &getTopology() const { return _topology; }

  private:

  /**
   * Instance Identifier
   */
  const HiCR::L0::Topology _topology;
};

} // namespace HiCR::L0
