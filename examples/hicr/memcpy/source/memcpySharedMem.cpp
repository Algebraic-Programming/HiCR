#include <hicr.hpp>
#include <hicr/backends/sharedMemory/pthreads/pthreads.hpp>
#include <iostream>

using namespace HiCR::backend::sharedMemory;

/*
This example uses HiCR (without frontends like TaskR)
to get the first backend found (assuming it is the shared memory
backend), return all memory spaces of that backend (assuming
it is the same as the NUMA nodes), and copy a block of 100 chars
from the first to the last NUMA domain, relying on the HiCR API. In case
the machine only has one NUMA node, it will copy data within this node.
*/

int main(int argc, char **argv)
{
  pthreads::Pthreads pthreadsBackend;
  pthreadsBackend.queryResources();

  auto memSpaceList = pthreadsBackend.getMemorySpaceList();
  const size_t firstNuma = 0;
  const size_t lastNuma = memSpaceList.size() - 1;
  auto memSpace1 = static_cast<pthreads::SharedMemorySpace &>(*memSpaceList[firstNuma]);
  auto memSpace2 = static_cast<pthreads::SharedMemorySpace &>(*memSpaceList[lastNuma]);
  auto slot1 = memSpace1.allocateMemorySlot(100);

  char *dataPtr = static_cast<char *>(slot1.getPointer());
  for (char *it = dataPtr; it < dataPtr + 100; it++) *it = 'c';

  auto slot2 = memSpace2.allocateMemorySlot(100);
  auto tag = memSpace1.createTag(0);

  // Non-blocking memcpy call, followed by fence guaranteeing completion
  pthreadsBackend.nb_memcpy(slot2, 0, 0, slot1, 0, 0, 100, tag);
  pthreadsBackend.wait(tag);

  dataPtr = static_cast<char *>(slot2.getPointer());
  for (char *it = dataPtr; it < dataPtr + 100; it++) assert(*it == 'c');

  return 0;
}
