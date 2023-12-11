#include <backends/ascend/core.hpp>
#include <backends/ascend/L1/memoryManager.hpp>
#include "include/telephoneGame.hpp"

int main(int argc, char **argv)
{
  // Initialize ACL runtime
  HiCR::backend::ascend::Core ascendCore;
  ascendCore.init();

  // Instantiating Memory manager
  HiCR::backend::ascend::L1::MemoryManager m(ascendCore);

  // Asking the memory manager to check the available resources
  m.queryMemorySpaces();

  // Obtaining memory spaces
  auto memSpaces = m.getMemorySpaceList();

  // Get the memory space id associated with the host
  auto memoryHostId = m.getHostId(memSpaces);
  // Make memory spaces contain only ascend device ids
  memSpaces.erase(memoryHostId);

  // Define the order of mem spaces for the telephone game
  auto memSpaceOrder = std::vector<HiCR::L1::MemoryManager::memorySpaceId_t>{};
  memSpaceOrder.emplace_back(memoryHostId);
  memSpaceOrder.insert(memSpaceOrder.end(), memSpaces.begin(), memSpaces.end());
  memSpaceOrder.emplace_back(memoryHostId);

  // Allocate and populate input memory slot
  auto input = m.allocateLocalMemorySlot(memoryHostId, BUFFER_SIZE);
  sprintf((char *)input->getPointer(), "Hello, HiCR user!\n");

  // Run the telephone game
  telephoneGame(m, input, memSpaceOrder, 3);

  // Free input memory slot
  m.freeLocalMemorySlot(input);

  // Finalize ACL
  ascendCore.finalize();
  return 0;
}
