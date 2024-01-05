#include <vector>
#include <memory>
#include <hicr/L0/memorySpace.hpp>
#include <hicr/L0/localMemorySlot.hpp>
#include <hicr/L1/memoryManager.hpp>
#include <hicr/L1/communicationManager.hpp>

#define BUFFER_SIZE 256
#define ITERATIONS 3
#define DST_OFFSET 0
#define SRC_OFFSET 0

void telephoneGame(HiCR::L1::MemoryManager &m, HiCR::L1::CommunicationManager &c, std::shared_ptr<HiCR::L0::LocalMemorySlot> input, std::vector<std::shared_ptr<HiCR::L0::MemorySpace>> memSpaces, int iterations)
{
  // Collect the newly created memory slots
  auto memSlots = std::vector<std::shared_ptr<HiCR::L0::LocalMemorySlot>>{};

  // iterate all over the memory spaces and create multiple memory slots in each one
  for (const auto &memSpace : memSpaces)
    for (int i = 0; i < iterations; i++)
      memSlots.emplace_back(m.allocateLocalMemorySlot(memSpace, BUFFER_SIZE));

  // Getting input memory slot
  auto srcMemSlot = input;

  // For each of the detected memory slots...
  for (auto dstMemSlot : memSlots)
  {
    // Perform the memcpy operations
    c.memcpy(dstMemSlot, DST_OFFSET, srcMemSlot, SRC_OFFSET, BUFFER_SIZE);

    // fence when the memcpy happens between two different memory spaces
    c.fence(0);

    // update source memory slot
    srcMemSlot = dstMemSlot;
  }

  // Getting output memory slot (the last one in the list)
  auto output = *memSlots.rbegin();

  // print the output of the telephone game
  printf("Input: %s\n", (const char *)input->getPointer());
  printf("Output: %s\n", (const char *)output->getPointer());

  // free the memory slots
  for (auto &memSlot : memSlots) m.freeLocalMemorySlot(memSlot);
}