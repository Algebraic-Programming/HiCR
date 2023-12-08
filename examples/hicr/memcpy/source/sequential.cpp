#include "include/telephoneGame.hpp"
#include <hicr/backends/sequential/L1/memoryManager.hpp>

#define BUFFER_SIZE 256
#define DST_OFFSET 0
#define SRC_OFFSET 0

int main(int argc, char **argv)
{
  // Instantiating Shared Memory backend
  HiCR::backend::sequential::L1::MemoryManager m;

  // Asking backend to check the available resources
  m.queryMemorySpaces();

  // Obtaining memory spaces
  auto memSpaces = m.getMemorySpaceList();

  // Define the order of mem spaces for the telephone game
  auto memSpaceOrder = std::vector<HiCR::L1::MemoryManager::memorySpaceId_t>(memSpaces.begin(), memSpaces.end());
  
  // Allocating memory slots in different NUMA domains
  auto input = m.allocateLocalMemorySlot(*memSpaces.begin(), BUFFER_SIZE); // First NUMA Domain

  // Initializing values in memory slot 1
  sprintf((char *)input->getPointer(), "Hello, HiCR user!\n");

  // Run the telephono game
  telephoneGame(m, input, memSpaceOrder, 3);

  // Free input memory slot
  m.freeLocalMemorySlot(input);
  return 0;
}
