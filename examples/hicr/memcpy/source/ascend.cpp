#include <vector>
#include <backends/ascend/L1/memoryManager.hpp>
#include <backends/ascend/L1/deviceManager.hpp>
#include <backends/ascend/L1/communicationManager.hpp>
#include <backends/sequential/L1/memoryManager.hpp>
#include <backends/sequential/L1/deviceManager.hpp>
#include "include/telephoneGame.hpp"

int main(int argc, char **argv)
{
  // Initializing host device manager
  HiCR::backend::sequential::L1::DeviceManager hostDeviceManager;
  hostDeviceManager.queryDevices();
  auto hostDevice = *hostDeviceManager.getDevices().begin();

  // Getting access to the host memory space
  auto hostMemorySpace = *hostDevice->getMemorySpaceList().begin();

  // Initialize (Ascend's) ACL runtime
  aclError err = aclInit(NULL);
  if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Failed to initialize Ascend Computing Language. Error %d", err);

  // Initializing ascend device manager
  HiCR::backend::ascend::L1::DeviceManager ascendDeviceManager;
  ascendDeviceManager.queryDevices();
  auto ascendDevices = ascendDeviceManager.getDevices();

  // Getting access to all ascend devices memory spaces
  std::vector<HiCR::L0::MemorySpace*> ascendMemorySpaces;
  for (const auto d : ascendDevices)
   for (const auto m : d->getMemorySpaceList())
     ascendMemorySpaces.push_back(m);

  // Define the order of mem spaces for the telephone game
  std::vector<HiCR::L0::MemorySpace*> memSpaceOrder;
  memSpaceOrder.emplace_back(hostMemorySpace);
  memSpaceOrder.insert(memSpaceOrder.end(), ascendMemorySpaces.begin(), ascendMemorySpaces.end());
  memSpaceOrder.emplace_back(hostMemorySpace);

  // Allocate and populate input memory slot
  HiCR::backend::sequential::L1::MemoryManager hostMemoryManager;
  auto input = hostMemoryManager.allocateLocalMemorySlot(hostMemorySpace, BUFFER_SIZE);
  sprintf((char *)input->getPointer(), "Hello, HiCR user!\n");

  // Instantiating Ascend memory and communication managers
  HiCR::backend::ascend::L1::MemoryManager ascendMemoryManager;
  HiCR::backend::ascend::L1::CommunicationManager ascendCommunicationManager;

  // Run the telephone game
  telephoneGame(ascendMemoryManager, ascendCommunicationManager, input, memSpaceOrder, 3);

  // Free input memory slot
  hostMemoryManager.freeLocalMemorySlot(input);

  // Finalize ACL
  err = aclFinalize();
  if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Failed to finalize Ascend Computing Language. Error %d", err);

  return 0;
}
