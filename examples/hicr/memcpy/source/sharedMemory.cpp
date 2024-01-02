#include "include/telephoneGame.hpp"
#include <backends/sharedMemory/hwloc/L1/memoryManager.hpp>
#include <backends/sharedMemory/hwloc/L1/communicationManager.hpp>
#include <backends/sharedMemory/hwloc/L1/deviceManager.hpp>

int main(int argc, char **argv)
{
  // Creating HWloc topology object
  hwloc_topology_t topology;

  // Reserving memory for hwloc
  hwloc_topology_init(&topology);

  // Initializing backend's device manager
  HiCR::backend::sharedMemory::hwloc::L1::DeviceManager dm(&topology);

  // Instantiating shared memory backend's memory manager
  HiCR::backend::sharedMemory::hwloc::L1::MemoryManager m(&topology);

  // Instantiating shared memory backend's communication manager
  HiCR::backend::sharedMemory::hwloc::L1::CommunicationManager c;

  // Asking backend to check the available devices
  dm.queryDevices();

  // Getting first device found
  auto d = *dm.getDevices().begin();

  // Obtaining memory spaces
  auto memSpaces = d->getMemorySpaceList();

  // Define the order of mem spaces for the telephone game
  auto memSpaceOrder = std::vector<HiCR::L0::MemorySpace*>(memSpaces.begin(), memSpaces.end());

  // Allocating memory slots in different NUMA domains
  auto input = m.allocateLocalMemorySlot(*memSpaces.begin(), BUFFER_SIZE); // First NUMA Domain

  // Initializing values in memory slot 1
  sprintf((char *)input->getPointer(), "Hello, HiCR user!\n");

  // Run the telephone game
  telephoneGame(m, c, input, memSpaceOrder, ITERATIONS);

  // Free input memory slot
  m.freeLocalMemorySlot(input);

  return 0;
}
