#include <hicr.hpp>
#include <hicr/backends/sharedMemory/sharedMemory.hpp>
#include <iostream>

using namespace HiCR::backend::sharedMemory;

/*
This example uses HiCR get the first backend found (assuming it is the shared memory
backend), return all memory spaces of that backend (assuming
it is the same as the NUMA nodes), and copy a block of 100 chars
from the first to the last NUMA domain, relying on the HiCR API. In case
the machine only has one NUMA node, it will copy data within this node.
*/

int main(int argc, char **argv)
{
  SharedMemory backend;
  backend.queryResources();

  auto memSpaceList = backend.getMemorySpaceList();

  const size_t firstNuma = 0;
  const size_t lastNuma = memSpaceList.size() - 1;

  auto memSpace1 = memSpaceList[firstNuma];
  auto memSpace2 = memSpaceList[lastNuma];

  auto slot1 = backend.allocateMemorySlot(memSpace1, 100);

  auto dataPtr1 = (char*)slot1.getPointer();
  for (size_t i = 0; i < 100; i++) dataPtr1[i] = 'c';

  auto slot2 = backend.allocateMemorySlot(memSpace2, 100);
  auto tag = backend.createTag();

  // Non-blocking memcpy call, followed by fence guaranteeing completion
  backend.memcpy(slot2, 0, slot1, 0, 100, tag);
  backend.fence(tag);

  auto dataPtr2 = (char*)slot2.getPointer();
  for (size_t i = 0; i < 100; i++) assert(dataPtr2[i] == 'c');

  return 0;
}
