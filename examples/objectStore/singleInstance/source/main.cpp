#include <hicr/backends/hwloc/L1/topologyManager.hpp>
#include <hicr/backends/hwloc/L1/memoryManager.hpp>
#include <hicr/backends/pthreads/L1/communicationManager.hpp>

#include <hicr/frontends/objectStore/objectStore.hpp>

#define OBJECT_STORE_TAG 42

// This simple example initiates locally an onbject store and checks local manipulation of blocks

int main(int argc, char **argv)
{
  // Creating HWloc topology object
  hwloc_topology_t topology;

  // Reserving memory for hwloc
  hwloc_topology_init(&topology);

  // Using default instance, communication and memory manager for single instance
  auto communicationManager = std::make_unique<HiCR::backend::pthreads::L1::CommunicationManager>();
  auto memoryManager        = std::make_unique<HiCR::backend::hwloc::L1::MemoryManager>(&topology);

  // Using HWLoc as topology managers
  std::vector<HiCR::L1::TopologyManager *> topologyManagers;
  auto                                     hwlocTopologyManager = std::make_unique<HiCR::backend::hwloc::L1::TopologyManager>(&topology);
  topologyManagers.push_back(hwlocTopologyManager.get());

  auto t = hwlocTopologyManager->queryTopology();

  // Getting first device found, all these steps are required to acquire the MemorySpace we will use in all other parts
  auto devices  = *t.getDevices().begin();
  auto memSpace = *devices->getMemorySpaceList().begin();

  // Initialize our objectStore Manager instance
  auto objectStoreManager = HiCR::objectStore::ObjectStore(*communicationManager, OBJECT_STORE_TAG, *memoryManager, memSpace, 0 /* instanceId */);

  // Allocate Memory (4KB)  for our dummy block; we could simply malloc here but chose to follow HiCR tooling strictly
  auto myBlockSlot = memoryManager->allocateLocalMemorySlot(memSpace, 4096);

  // Get the raw pointer
  char *myBlockData = (char *)myBlockSlot->getPointer();

  // Initialize the block with an 'R' for 'Random'
  myBlockData[0] = 82;

  // Publish the block with arbitrary ID 1
  std::shared_ptr<HiCR::objectStore::DataObject> myBlock = std::make_shared<HiCR::objectStore::DataObject>(objectStoreManager.createObject(myBlockSlot, 1));

  objectStoreManager.publish(myBlock);

  // Get the raw pointers through the block
  char *myBlockData1 = (char *)objectStoreManager.get(*myBlock)->getPointer(); // local, so no need to fence in this example

  // Test if we read correctly after all this manoeuvering
  printf("Block 1: %c\n", myBlockData1[0]);

  // Change the block to an 'S' for 'Sentinel'
  myBlockData[0] = 83;

  // Re-publish the updated block
  std::shared_ptr<HiCR::objectStore::DataObject> myBlock2 = std::make_shared<HiCR::objectStore::DataObject>(objectStoreManager.createObject(myBlockSlot, 2));
  objectStoreManager.publish(myBlock2);

  auto  myBlockSlot2 = objectStoreManager.get(*myBlock2);
  char *myBlockData2 = (char *)myBlockSlot2->getPointer();

  // Test again, now through the copied block
  printf("Block 2: %c\n", myBlockData2[0]);

  memoryManager->freeLocalMemorySlot(myBlockSlot);

  // Now experiment with an allocated block
  // We need a slot to create a block into
  std::shared_ptr<HiCR::L0::LocalMemorySlot> customMemorySlot = memoryManager->allocateLocalMemorySlot(memSpace, 4096);

  // Allocate our block in the given memory slot
  HiCR::objectStore::DataObject customBlock = objectStoreManager.createObject(customMemorySlot, 3);

  // Get the raw pointer
  char *customBlockData = (char *)customMemorySlot->getPointer();
  customBlockData[0]    = 'T';
  customBlockData[1]    = 'e';
  customBlockData[2]    = 's';
  customBlockData[3]    = 't';
  customBlockData[4]    = '\0';

  // Make a copy of our block; it should point to the same data
  HiCR::objectStore::DataObject customBlock2 = customBlock;

  std::shared_ptr<HiCR::objectStore::DataObject> customBlock2Ptr = std::make_shared<HiCR::objectStore::DataObject>(customBlock2);
  objectStoreManager.publish(customBlock2Ptr);

  auto customBlockSlot2 = objectStoreManager.get(*customBlock2Ptr);

  char *customBlockData2 = (char *)customBlockSlot2->getPointer();

  // Verify our copy shows the same data
  printf("Copy of a Custom Block: %s\n", customBlockData2);

  // Delete the blocks
  objectStoreManager.destroy(*myBlock2);

  hwloc_topology_destroy(topology);

  return 0;
}
