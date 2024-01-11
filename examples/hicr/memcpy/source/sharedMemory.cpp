#include "include/telephoneGame.hpp"
#include <backends/host/hwloc/L1/memoryManager.hpp>
#include <backends/host/pthreads/L1/communicationManager.hpp>
#include <backends/host/hwloc/L1/topologyManager.hpp>

int main(int argc, char **argv)
{
  // Creating HWloc topology object
  hwloc_topology_t topology;

  // Reserving memory for hwloc
  hwloc_topology_init(&topology);

  // Initializing host (CPU) topology manager
  HiCR::backend::host::hwloc::L1::TopologyManager dm(&topology);

  // Instantiating host (CPU) memory manager
  HiCR::backend::host::hwloc::L1::MemoryManager m(&topology);

  // Instantiating host (CPU) communication manager
  HiCR::backend::host::pthreads::L1::CommunicationManager c;

  // Asking backend to check the available devices
  const auto t = dm.queryTopology();

  // Getting first device found
  auto d = *t.getDevices().begin();

  // Obtaining memory spaces
  auto memSpaces = d->getMemorySpaceList();

  // Define the order of mem spaces for the telephone game
  auto memSpaceOrder = std::vector<std::shared_ptr<HiCR::L0::MemorySpace>>(memSpaces.begin(), memSpaces.end());

  // Allocating memory slots in different NUMA domains
  auto firstMemSpace = *memSpaces.begin();
  auto input = m.allocateLocalMemorySlot(firstMemSpace, BUFFER_SIZE); // First NUMA Domain

  // Initializing values in memory slot 1
  sprintf((char *)input->getPointer(), "Hello, HiCR user!\n");

  // Run the telephone game
  telephoneGame(m, c, input, memSpaceOrder, ITERATIONS);

  // Free input memory slot
  m.freeLocalMemorySlot(input);

  return 0;
}
