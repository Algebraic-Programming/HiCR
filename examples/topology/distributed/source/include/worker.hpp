#pragma once

#include <hicr/core/L0/memorySpace.hpp>
#include <hicr/core/L0/topology.hpp>
#include <hicr/core/L1/topologyManager.hpp>
#include <hicr/core/L1/instanceManager.hpp>
#include <hicr/core/L0/topology.hpp>
#include <hicr/backends/pthreads/L1/computeManager.hpp>
#include "common.hpp"

void sendTopology(HiCR::frontend::RPCEngine &rpcEngine)
{
  // Storage for the topology to send
  HiCR::L0::Topology workerTopology;

  // List of topology managers to query
  std::vector<std::shared_ptr<HiCR::L1::TopologyManager>> topologyManagerList;

// Now instantiating topology managers (which ones is determined by backend availability during compilation)
#ifdef _HICR_USE_HWLOC_BACKEND_

  // Creating HWloc topology object
  hwloc_topology_t topology;

  // Reserving memory for hwloc
  hwloc_topology_init(&topology);

  // Initializing HWLoc-based host (CPU) topology manager
  auto hwlocTopologyManager = std::make_shared<HiCR::backend::hwloc::L1::TopologyManager>(&topology);

  // Adding topology manager to the list
  topologyManagerList.push_back(hwlocTopologyManager);

#endif // _HICR_USE_HWLOC_BACKEND_

#ifdef _HICR_USE_ASCEND_BACKEND_

  // Initialize (Ascend's) ACL runtime
  aclError err = aclInit(NULL);
  if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Failed to initialize Ascend Computing Language. Error %d", err);

  // Initializing ascend topology manager
  auto ascendTopologyManager = std::make_shared<HiCR::backend::ascend::L1::TopologyManager>();

  // Adding topology manager to the list
  topologyManagerList.push_back(ascendTopologyManager);

#endif // _HICR_USE_ASCEND_BACKEND_

  // For each topology manager detected
  for (const auto &tm : topologyManagerList)
  {
    // Getting the topology information from the topology manager
    const auto t = tm->queryTopology();

    // Merging its information to the worker topology object to send
    workerTopology.merge(t);
  }

  // Serializing theworker topology and dumping it into a raw string message
  auto message = workerTopology.serialize().dump();

  // Registering return value
  rpcEngine.submitReturnValue(message.data(), message.size() + 1);
}

void workerFc(HiCR::frontend::RPCEngine &rpcEngine)
{
  // Creating compute manager (responsible for executing the RPC)
  HiCR::backend::pthreads::L1::ComputeManager cpm;

  // Creating execution unit to run as RPC
  auto executionUnit = std::make_shared<HiCR::backend::pthreads::L0::ExecutionUnit>([&](void *closure) { sendTopology(rpcEngine); });

  // Adding RPC target by name and the execution unit id to run
  rpcEngine.addRPCTarget(TOPOLOGY_RPC_NAME, executionUnit);

  // Listening for RPC requests
  rpcEngine.listen();
}
