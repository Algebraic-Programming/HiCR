#include "include/telephoneGame.hpp"
#include <backends/ascend/L1/memoryManager.hpp>
#include <backends/ascend/common.hpp>
#include <backends/ascend/core.hpp>

using namespace HiCR::backend;

int main(int argc, char **argv)
{
  // Initialize ACL runtime
  ascend::Core ascendCore;
  ascendCore.init();

  // Instantiating Memory manager
  ascend::L1::MemoryManager m(ascendCore);

  // Asking the memory manager to check the available resources
  m.queryMemorySpaces();

  // Obtaining memory spaces
  auto memSpaces = m.getMemorySpaceList();

  // Get all memory spaces associated with Ascend devices
  std::vector<ascend::L0::MemorySpace*> deviceMemSpaces;
  for (const auto memSpace : memSpaces)
  {
    // Getting ascend type memory space pointer
    auto ascendMemSpace = (ascend::L0::MemorySpace*) memSpace;

    // Adding memory spaces that are not of host type
    if (ascendMemSpace->getDeviceType() != ascend::deviceType_t::Host) deviceMemSpaces.push_back(ascendMemSpace);
  }
  
  // Getting host memory space
  auto hostMemSpace = m.getHostId(memSpaces);

  // Define the order of mem spaces for the telephone game
  auto memSpaceOrder = std::vector<HiCR::L0::MemorySpace*>{};
  memSpaceOrder.emplace_back(hostMemSpace);
  memSpaceOrder.insert(memSpaceOrder.end(), deviceMemSpaces.begin(), deviceMemSpaces.end());
  memSpaceOrder.emplace_back(hostMemSpace);

  // Allocate and populate input memory slot
  auto input = m.allocateLocalMemorySlot(hostMemSpace, BUFFER_SIZE);
  sprintf((char *)input->getPointer(), "Hello, HiCR user!\n");

  // Run the telephone game
  telephoneGame(m, input, memSpaceOrder, 3);

  // Free input memory slot
  m.freeLocalMemorySlot(input);

  // Finalize ACL
  ascendCore.finalize();
  return 0;
}
