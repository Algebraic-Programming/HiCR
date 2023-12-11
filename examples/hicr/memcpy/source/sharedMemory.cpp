#include "include/telephoneGame.hpp"
#include <backends/sharedMemory/L1/memoryManager.hpp>

#define BUFFER_SIZE 256
#define DST_OFFSET 0
#define SRC_OFFSET 0

int main(int argc, char **argv)
{
  // Creating HWloc topology object
  hwloc_topology_t topology;

  // Reserving memory for hwloc
  hwloc_topology_init(&topology);

  // Instantiating Shared Memory backend
  HiCR::backend::sharedMemory::L1::MemoryManager m(&topology);

  // Asking backend to check the available resources
  m.queryMemorySpaces();

  // Obtaining memory spaces
  auto memSpaces = m.getMemorySpaceList();

  // Define the order of mem spaces for the telephone game
  auto memSpaceOrder = std::vector<HiCR::L1::MemoryManager::memorySpaceId_t>(memSpaces.begin(), memSpaces.end());

  // Specific to the shared memory backend: Adjusting memory binding support to the system's
  m.setRequestedBindingType(m.getSupportedBindingType(*memSpaces.begin()));

  // Allocating memory slots in different NUMA domains
  auto input = m.allocateLocalMemorySlot(*memSpaces.begin(), BUFFER_SIZE); // First NUMA Domain

  // Initializing values in memory slot 1
  sprintf((char *)input->getPointer(), "Hello, HiCR user!\n");

  // Run the telephone game
  telephoneGame(m, input, memSpaceOrder, 3);

  // Free input memory slot
  m.freeLocalMemorySlot(input);

  return 0;
}
