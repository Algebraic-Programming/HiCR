#include "include/telephoneGame.hpp"
#include <backends/sequential/L1/topologyManager.hpp>
#include <backends/sequential/L1/memoryManager.hpp>
#include <backends/sequential/L1/communicationManager.hpp>

int main(int argc, char **argv)
{
  // Initializing backend's device manager
  HiCR::backend::sequential::L1::TopologyManager dm;

  // Instantiating sequential backend's memory manager
  HiCR::backend::sequential::L1::MemoryManager m;

  // Instantiating sequential backend's communication manager
  HiCR::backend::sequential::L1::CommunicationManager c;

  // Asking backend to check the available devices
  dm.queryDevices();

  // Getting first device found
  auto d = *dm.getDevices().begin();

  // Obtaining memory spaces
  auto memSpaces = d->getMemorySpaceList();

  // Define the order of mem spaces for the telephone game
  auto memSpaceOrder = std::vector<HiCR::L0::MemorySpace*>(memSpaces.begin(), memSpaces.end());

  // Allocating memory slots in different NUMA domains
  auto input = m.allocateLocalMemorySlot(*memSpaces.begin(), BUFFER_SIZE); // First NUMA Domain

  // Initializing values in memory slot 1
  sprintf((char *)input->getPointer(), "Hello, HiCR user!\n");

  // Run the telephono game
  telephoneGame(m, c, input, memSpaceOrder, 3);

  // Free input memory slot
  m.freeLocalMemorySlot(input);
  return 0;
}
