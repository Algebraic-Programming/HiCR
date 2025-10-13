#pragma once

#include <hicr/core/instance.hpp>
#include <hicr/core/instanceManager.hpp>
#include <hicr/core/topology.hpp>

/**
 * Create new HiCR instances
 * 
 * \param[in] instanceManager
 * \param[in] instanceCount
 * \param[in] topology
*/
void createInstances(HiCR::InstanceManager &instanceManager, size_t instanceCount, HiCR::Topology &topology)
{
  auto instanceTemplate = instanceManager.createInstanceTemplate(topology);

  for (size_t i = 0; i < instanceCount; i++)
  {
    auto instance = instanceManager.createInstance(*instanceTemplate);
    printf("[Instance %lu] Create instance %lu\n", instanceManager.getCurrentInstance()->getId(), instance->getId());
  }
}

/**
 * Function that all the created instances should execute
 * 
 * \param[in] instanceManager
*/
void workerFc(HiCR::InstanceManager &instanceManager) { printf("[Instance %lu] Hello World\n", instanceManager.getCurrentInstance()->getId()); }