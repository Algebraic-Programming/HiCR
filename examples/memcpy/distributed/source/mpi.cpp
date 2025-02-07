#include <iostream>
#include <mpi.h>
#include <hicr/backends/mpi/L1/instanceManager.hpp>
#include <hicr/backends/mpi/L1/memoryManager.hpp>
#include <hicr/backends/mpi/L1/communicationManager.hpp>
#include <hicr/backends/hwloc/L1/topologyManager.hpp>
#include "include/remoteMemcpy.hpp"

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

  // Creating memory and communication managers
  HiCR::backend::mpi::L1::MemoryManager        mm;
  HiCR::backend::mpi::L1::CommunicationManager cc;

  // Running the remote memcpy example
  remoteMemcpy(im.get(), &tm, &mm, &cc);

  // Finalizing instance manager
  im->finalize();
}
