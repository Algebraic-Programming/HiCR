#pragma once

#include <hicr/core/instance.hpp>
#include <hicr/core/instanceManager.hpp>
#include <hicr/core/topology.hpp>

void createInstances(HiCR::InstanceManager &im, size_t instanceCount, HiCR::Topology &t)
{
  auto instanceTemplate = im.createInstanceTemplate(t);

  for (size_t i = 0; i < instanceCount; i++)
  {
    auto instance = im.createInstance(*instanceTemplate);
    printf("[Instance %lu] Create instance %lu\n", im.getCurrentInstance()->getId(), instance->getId());
  }
}