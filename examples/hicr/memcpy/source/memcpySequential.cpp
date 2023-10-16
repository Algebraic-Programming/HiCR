#include <hicr/backends/sequential/memoryManager.hpp>
#include <hicr/backends/sequential/dataMover.hpp>
#include <hicr.hpp>

#define BUFFER_SIZE 256
#define DST_OFFSET 0
#define SRC_OFFSET 0

int main(int argc, char **argv)
{
 // Instantiating Shared Memory backend
 HiCR::backend::sequential::DataMover d;
 HiCR::backend::sequential::MemoryManager m;

 // Asking backend to check the available resources
 m.queryMemorySpaces();

 // Obtaining memory spaces
 auto memSpaces = m.getMemorySpaceList();

 // Allocating memory slots in different NUMA domains
 auto slot1 = m.allocateLocalMemorySlot(*memSpaces.begin(),   BUFFER_SIZE); // First NUMA Domain
 auto slot2 = m.allocateLocalMemorySlot(*memSpaces.end() - 1, BUFFER_SIZE); // Last NUMA Domain

 // Initializing values in memory slot 1
 sprintf((char*)slot1->getPointer(), "Hello, HiCR user!\n");

 // Performing the copy
 d.memcpy(slot2, DST_OFFSET, slot1, SRC_OFFSET, BUFFER_SIZE);

 // Waiting on the operation to have finished
 d.fence(0);

 // Checking whether the copy was successful
 printf("%s", (const char*)slot2->getPointer());

 return 0;
}

