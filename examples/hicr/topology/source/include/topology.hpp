#pragma once

#include <hicr/L1/topologyManager.hpp>

void topologyFc(HiCR::L1::TopologyManager &topologyManager)
{
  // Querying the devices that this topology manager can detect
  auto topology = topologyManager.queryTopology();

  // Now summarizing the devices seen by this topology manager
  for (const auto &d : topology.getDevices())
  {
    printf("  + '%s'\n", d->getType().c_str());
    printf("    Compute Resources: %lu %s(s)\n", d->getComputeResourceList().size(), (*d->getComputeResourceList().begin())->getType().c_str());
    for (const auto &m : d->getMemorySpaceList()) printf("    Memory Space:     '%s', %f Gb\n", m->getType().c_str(), (double)m->getSize() / (double)(1024ul * 1024ul * 1024ul));
  }
}
