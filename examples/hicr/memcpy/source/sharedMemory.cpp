#include <hicr.hpp>
#include <hicr/backends/sharedMemory/memoryManager.hpp>

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
  HiCR::backend::sharedMemory::MemoryManager m(&topology);

  // Asking backend to check the available resources
  m.queryMemorySpaces();

  // Obtaining memory spaces
  auto memSpaces = m.getMemorySpaceList();

  // Specific to the shared memory backend: Adjusting memory binding support to the system's
  m.setRequestedBindingType(m.getSupportedBindingType(*memSpaces.begin()));

  // Allocating memory slots in different NUMA domains
  auto slot1 = m.allocateLocalMemorySlot(*memSpaces.begin(), BUFFER_SIZE);  // First NUMA Domain
  auto slot2 = m.allocateLocalMemorySlot(*memSpaces.rbegin(), BUFFER_SIZE); // Last NUMA Domain

  // Initializing values in memory slot 1
  sprintf((char *)slot1->getPointer(), "Hello, HiCR user!\n");

  // Performing the copy
  m.memcpy(slot2, DST_OFFSET, slot1, SRC_OFFSET, BUFFER_SIZE);

  // Waiting on the operation to have finished
  m.fence(0);

  // Checking whether the copy was successful
  printf("%s", (const char *)slot2->getPointer());

  return 0;
}
