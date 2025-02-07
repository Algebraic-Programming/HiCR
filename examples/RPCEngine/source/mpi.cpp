#include <mpi.h>
#include <hicr/backends/mpi/L1/instanceManager.hpp>
#include <hicr/backends/mpi/L1/memoryManager.hpp>
#include <hicr/backends/mpi/L1/communicationManager.hpp>
#include <hicr/backends/pthreads/L1/computeManager.hpp>
#include <hicr/backends/pthreads/L0/executionUnit.hpp>
#include <hicr/backends/hwloc/L1/topologyManager.hpp>
#include "include/RPCTest.hpp"

int main(int argc, char **argv)
{
  // Initializing instance manager
  auto im = HiCR::backend::mpi::L1::InstanceManager::createDefault(&argc, &argv);

  // Creating HWloc topology object
  hwloc_topology_t topology;

  // Reserving memory for hwloc
  hwloc_topology_init(&topology);

  // Initializing host (CPU) topology manager
  HiCR::backend::hwloc::L1::TopologyManager tm(&topology);

  // Creating compute manager (responsible for executing the RPC)
  HiCR::backend::pthreads::L1::ComputeManager cpm;

  // Creating memory and communication managers (buffering and communication)
  HiCR::backend::mpi::L1::MemoryManager        mm;
  HiCR::backend::mpi::L1::CommunicationManager cc;

  // Gathering topology from the topology manager
  const auto t = tm.queryTopology();

  // Selecting first device
  auto d = *t.getDevices().begin();

  // Getting memory space list from device
  auto memSpaces = d->getMemorySpaceList();

  // Grabbing first memory space for buffering
  auto bufferMemorySpace = *memSpaces.begin();

  // Now getting compute resource list from device
  auto computeResources = d->getComputeResourceList();

  // Grabbing first compute resource for computing incoming RPCs
  auto executeResource = *computeResources.begin();

  // Creating execution unit to run as RPC 
  auto executionUnit = std::make_shared<HiCR::backend::pthreads::L0::ExecutionUnit>([&im](void* closure)
   { printf("Instance %lu: running Test RPC\n", im->getCurrentInstance()->getId()); });

  // Calling common function for printing the topology
  RPCTestFc(cc, mm, cpm, *im, bufferMemorySpace, executeResource, executionUnit);

  // Finalizing instance manager
  im->finalize();

  return 0;
}
