#include <hicr/backends/memoryManager.hpp>
#include <hicr/memorySlot.hpp>
#include <vector>

#define BUFFER_SIZE 256
#define DST_OFFSET 0
#define SRC_OFFSET 0

void telephoneGame(HiCR::backend::MemoryManager &m, HiCR::MemorySlot *input, std::vector<HiCR::backend::MemoryManager::memorySpaceId_t> memSpaces, int memcpyInMemspace)
{
  // Asking Memory manager to check the available resources
  m.queryMemorySpaces();

  // Obtaining memory spaces
  // TODO: what do we do with these
  auto memSpacesO = m.getMemorySpaceList();
  (void)memSpacesO;

  // Collect the newly created memory slots
  auto memSlots = std::vector<HiCR::MemorySlot *>{};

  // iterate all over the memory spaces and create multiple memory slots in each one
  for (HiCR::backend::MemoryManager::memorySpaceId_t memSpace : memSpaces)
  {
    for (int i = 0; i < memcpyInMemspace; i++)
    {
      auto memSlot = m.allocateLocalMemorySlot(memSpace, BUFFER_SIZE);

      memSlots.emplace_back(memSlot);
    }
  }

  HiCR::MemorySlot *srcMemSlot = input;
  HiCR::MemorySlot *output = *memSlots.rbegin();
  int memcpyCount = 0;
  // perform the memcpy operations
  for (const auto dstMemSlot : memSlots)
  {
    m.memcpy(dstMemSlot, DST_OFFSET, srcMemSlot, SRC_OFFSET, BUFFER_SIZE);

    // fence when the memcpy happens between two different memory spaces
    if (memcpyCount % memcpyInMemspace == 0 && memcpyCount > 0) m.fence(0);

    // update source memory slot
    srcMemSlot = dstMemSlot;
    memcpyCount++;
  }

  // print the output of the telephone game
  printf("Input: %s\n", (const char *)input->getPointer());
  printf("Output: %s\n", (const char *)output->getPointer());

  // free the memory slots
  for (const auto memSlot : memSlots) m.freeLocalMemorySlot(memSlot);
}