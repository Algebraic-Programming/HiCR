#include <hicr/backends/ascend/init.hpp>
#include <hicr/backends/ascend/memoryManager.hpp>

#define BUFFER_SIZE 256
#define DST_OFFSET 0
#define SRC_OFFSET 0

int main(int argc, char **argv)
{
  // Initialize ACL runtime
  HiCR::backend::ascend::Initializer i;
  i.init();

  // Instantiating Shared Memory m
  HiCR::backend::ascend::MemoryManager m(i);

  // Asking m to check the available resources
  m.queryMemorySpaces();

  // Obtaining memory spaces
  auto memSpaces = m.getMemorySpaceList();

  // get the memory space id associated with the host
  auto memoryHostId = m.getHostId(memSpaces);
  // make memory spaces contain only device ids
  memSpaces.erase(memoryHostId); 
  auto memoryDeviceId0 = *memSpaces.begin();
  auto memoryDeviceId1 = *memSpaces.rbegin();


  // Allocating memory slots in different Ascend devices and on host
  auto hostSlot1 = m.allocateLocalMemorySlot(memoryHostId, BUFFER_SIZE);              // initial local host allocation
  auto ascendSlot1Device0 = m.allocateLocalMemorySlot(memoryDeviceId0, BUFFER_SIZE);      // first allocation on Ascend device 0
  auto ascendSlot2Device0 = m.allocateLocalMemorySlot(memoryDeviceId0, BUFFER_SIZE);      // second allocation on Ascend device 0
  auto ascendSlot1Device1 = m.allocateLocalMemorySlot(memoryDeviceId1, BUFFER_SIZE); // first allocation on Ascend device 7
  auto hostSlot2 = m.allocateLocalMemorySlot(memoryHostId, BUFFER_SIZE);              // final local host allocation

  // populate starting host slot
  sprintf((char *)hostSlot1->getPointer(), "Hello, HiCR user!\n");

  // perform the memcpys
  m.memcpy(ascendSlot1Device0, DST_OFFSET, hostSlot1, SRC_OFFSET, BUFFER_SIZE);
  m.memcpy(ascendSlot2Device0, DST_OFFSET, ascendSlot1Device0, SRC_OFFSET, BUFFER_SIZE);
  m.memcpy(ascendSlot1Device1, DST_OFFSET, ascendSlot2Device0, SRC_OFFSET, BUFFER_SIZE);
  m.memcpy(hostSlot2, DST_OFFSET, ascendSlot1Device1, SRC_OFFSET, BUFFER_SIZE);

  // Checking whether the copy was successful
  printf("start: %s\n", (const char *)hostSlot1->getPointer());
  printf("result: %s\n", (const char *)hostSlot2->getPointer());

  // deallocate memory slots (the destructor wil take care of that)
  m.freeLocalMemorySlot(hostSlot1);
  m.freeLocalMemorySlot(hostSlot2);
  m.freeLocalMemorySlot(ascendSlot1Device0);
  m.freeLocalMemorySlot(ascendSlot2Device0);
  m.freeLocalMemorySlot(ascendSlot1Device1);

  i.finalize();
  return 0;
}
