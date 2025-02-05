#pragma once

#include <hicr/core/L1/topologyManager.hpp>
#include <hicr/core/L1/communicationManager.hpp>
#include <hicr/core/L1/memoryManager.hpp>
#include <hicr/core/L1/instanceManager.hpp>
#include <hicr/frontends/RPCEngine/RPCEngine.hpp>

void RPCTestFc(HiCR::L1::CommunicationManager &cm, HiCR::L1::MemoryManager &mm, HiCR::L1::TopologyManager &tm, HiCR::L1::InstanceManager &im)
{
  // Creating RPC engine instance
  HiCR::frontend::RPCEngine rpcEngine(cm, im, mm, tm);

  // Initialize RPC engine
  rpcEngine.initialize();

  // Querying the devices that this topology manager can detect
  auto topology = tm.queryTopology();

  // Getting current instance
  auto currentInstance = im.getCurrentInstance();

  // If I'm instance zero, I request an RPC. Otherwise I listen
  if (currentInstance->getId() == 0) rpcEngine.requestRPC(*im.getInstances()[1], "Test RPC");
  else rpcEngine.listen();

  // Now summarizing the devices seen by this topology manager
  for (const auto &d : topology.getDevices())
  {
    printf("  + '%s'\n", d->getType().c_str());
    printf("    Compute Resources: %lu %s(s)\n", d->getComputeResourceList().size(), (*d->getComputeResourceList().begin())->getType().c_str());
    for (const auto &m : d->getMemorySpaceList()) printf("    Memory Space:     '%s', %f Gb\n", m->getType().c_str(), (double)m->getSize() / (double)(1024ul * 1024ul * 1024ul));
  }
}
