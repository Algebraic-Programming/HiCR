#include <hicr/backends/sharedMemory/sharedMemory.hpp>
#include <hicr.hpp>

#define BUFFER_SIZE 256
#define DST_OFFSET 0
#define SRC_OFFSET 0

int main(int argc, char **argv)
{
 // Instantiating Shared Memory backend
 HiCR::backend::sharedMemory::SharedMemory backend;

 // Asking backend to check the available resources
 backend.queryMemorySpaces();

 // Obtaining memory spaces
 auto memSpaces = backend.getMemorySpaceList();

 // Allocating memory slots in different NUMA domains
 auto slot1 = backend.allocateMemorySlot(*memSpaces.begin(),  BUFFER_SIZE); // First NUMA Domain
 auto slot2 = backend.allocateMemorySlot(*memSpaces.end() - 1, BUFFER_SIZE);   // Last NUMA Domain

 // Initializing values in memory slot 1
 sprintf((char*)backend.getMemorySlotPointer(slot1), "Hello, HiCR user!\n");

 // Performing the copy
 backend.memcpy(slot2, DST_OFFSET, slot1, SRC_OFFSET, BUFFER_SIZE);

 // Waiting on the operation to have finished
 backend.fence();

 // Checking whether the copy was successful
 printf("%s", (const char*)backend.getMemorySlotPointer(slot2));

 return 0;
}

