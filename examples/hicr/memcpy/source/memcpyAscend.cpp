#include <hicr/backends/ascend/ascend.hpp>
#include <hicr.hpp>

#define BUFFER_SIZE 256
#define DST_OFFSET 0
#define SRC_OFFSET 0

int main(int argc, char **argv)
{
  // Instantiating Shared Memory backend
  HiCR::backend::ascend::Ascend backend;

  // Asking backend to check the available resources
  backend.queryMemorySpaces();

  // Obtaining memory spaces
  auto memSpaces = backend.getMemorySpaceList();

  // Allocating memory slots in different Ascend devices and on host
  auto hostSlot1 = backend.allocateLocalMemorySlot(*memSpaces.end() - 1, BUFFER_SIZE);   // initial local host allocation  
  auto ascendSlot1Device0 = backend.allocateLocalMemorySlot(*memSpaces.begin(), BUFFER_SIZE);   // first allocation on Ascend device 0 
  auto ascendSlot2Device0 = backend.allocateLocalMemorySlot(*memSpaces.begin(), BUFFER_SIZE);   // second allocation on Ascend device 0 
  auto ascendSlot1Device7 = backend.allocateLocalMemorySlot(*memSpaces.end() - 2, BUFFER_SIZE); // first allocation on Ascend device 7
  auto hostSlot2 = backend.allocateLocalMemorySlot(*memSpaces.end() - 1, BUFFER_SIZE);   // final local host allocation

  // populate starting host slot
  sprintf((char *)hostSlot1->getPointer(), "Hello, HiCR user!\n");

  // perform the memcpys

  backend.memcpy(ascendSlot1Device0, DST_OFFSET, hostSlot1, SRC_OFFSET, BUFFER_SIZE);
  backend.memcpy(ascendSlot2Device0, DST_OFFSET, ascendSlot1Device0, SRC_OFFSET, BUFFER_SIZE);
  backend.memcpy(ascendSlot1Device7, DST_OFFSET, ascendSlot2Device0, SRC_OFFSET, BUFFER_SIZE);
  backend.memcpy(hostSlot2, DST_OFFSET, ascendSlot1Device7, SRC_OFFSET, BUFFER_SIZE);

  // Checking whether the copy was successful
  printf("start: %s\n", (const char *)hostSlot1->getPointer());
  printf("result: %s\n", (const char *)hostSlot2->getPointer());

  // deallocate memory slots (the destructor wil take care of that)
  backend.freeLocalMemorySlot(hostSlot1);   
  backend.freeLocalMemorySlot(hostSlot2);   
  backend.freeLocalMemorySlot(ascendSlot1Device0); 
  backend.freeLocalMemorySlot(ascendSlot2Device0); 
  backend.freeLocalMemorySlot(ascendSlot1Device7); 


  // Waiting on the operation to have finished
  // backend.fence(0);

  return 0;
}
