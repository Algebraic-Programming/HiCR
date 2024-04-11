#include <hwloc.h>
#include <hicr/frontends/runtime/runtime.hpp>
#include <hicr/backends/mpi/L1/instanceManager.hpp>
#include <hicr/backends/mpi/L1/communicationManager.hpp>
#include <hicr/backends/mpi/L1/memoryManager.hpp>
#include <hicr/backends/host/hwloc/L1/topologyManager.hpp>

#ifdef _HICR_USE_ASCEND_BACKEND_
  #include <hicr/backends/ascend/L1/topologyManager.hpp>
#endif

#include "../../common.hpp"

// Coordinator entry point functions
void coordinatorEntryPointFc(HiCR::Runtime &runtime)
{
  // Getting instance manager from the runtime
  auto instanceManager = runtime.getInstanceManager();

  // Getting coordinator instance
  auto coordinator = runtime.getCurrentInstance();

  // Creating a welcome message
  std::string welcomeMsg = "Hello from the coordinator";

  // Buffer for data objects to transfer
  std::vector<std::shared_ptr<HiCR::runtime::DataObject>> dataObjects;

  // Sending message to all the workers
  for (auto &instance : instanceManager->getInstances()) 
   if (instance->getId() != coordinator->getHiCRInstance()->getId())
    {
      printf("Coordinator (%lu) sending message to instance %lu\n", coordinator->getHiCRInstance()->getId(), instance->getId());
      // Creating data object with welcome message
      auto dataObject = coordinator->createDataObject(welcomeMsg.data(), welcomeMsg.size() + 1);

      // Getting data object identifier
      auto dataObjectId = dataObject->getId();

      // Publishing data object
      dataObject->publish();

      // Adding data object to the vector
      dataObjects.push_back(dataObject);

      // Sending message with only the data object identifier
      coordinator->sendMessage(instance->getId(), &dataObjectId, sizeof(HiCR::runtime::DataObject::dataObjectId_t));
    }

  // Sending a message to myself just to test self-comunication
  size_t workerCount = instanceManager->getInstances().size() - 1;
  coordinator->sendMessage(coordinator->getHiCRInstance()->getId(), &workerCount, sizeof(workerCount));
  auto message = coordinator->recvMessage(coordinator->getHiCRInstance()->getId());
  printf("[Coordinator] Received worker count: %lu from myself\n", *(size_t *)message.data);

  // Testing release completion for all data objects
  bool allDataObjectsReleased = false;
  while (allDataObjectsReleased == false)
  {
    allDataObjectsReleased = true;
    for (auto &dataObject : dataObjects) allDataObjectsReleased &= dataObject->release();
  }

  printf("Coordinator Reached End Function\n");
}

// Worker entry point function
void workerEntryPointFc(HiCR::Runtime &runtime, const std::string &entryPointName)
{
  printf("Hello, I am instance Id %lu, executing entry point '%s'\n", runtime.getInstanceId(), entryPointName.c_str());

  // Getting my current worker instance
  auto currentInstance = runtime.getCurrentInstance();

  // Iterating over all other instances to get a message from the coordinator
  HiCR::runtime::Instance::message_t message;
  while (message.size == 0)
      for (auto& instance : runtime.getInstanceManager()->getInstances())
      {
        message = currentInstance->recvMessageAsync(instance->getId());
        if (message.size > 0) break;
      }

  // Getting data object id from message
  const auto dataObjectId = *((HiCR::runtime::DataObject::dataObjectId_t *)message.data);

  // Printing data object id
  printf("[Worker %lu] Requesting data object id %u from coordinator.\n", runtime.getInstanceId(), dataObjectId);

  // Getting data object from coordinator
  auto dataObject = currentInstance->getDataObject(dataObjectId);

  // Printing data object contents
  printf("[Worker %lu] Received message from coordinator: '%s'\n", runtime.getInstanceId(), (const char *)dataObject->getData());

  // Freeing up internal buffer
  dataObject->destroyBuffer();
};

int main(int argc, char *argv[])
{
  // Using MPI as instance, communication and memory manager for multiple instances
  auto instanceManager = HiCR::backend::mpi::L1::InstanceManager::createDefault(&argc, &argv);
  auto communicationManager = std::make_unique<HiCR::backend::mpi::L1::CommunicationManager>();
  auto memoryManager = std::make_unique<HiCR::backend::mpi::L1::MemoryManager>();

  // Using HWLoc and Ascend (if configured) as topology managers
  std::vector<HiCR::L1::TopologyManager *> topologyManagers;
  auto hwlocTopologyManager = HiCR::backend::host::hwloc::L1::TopologyManager::createDefault();
  topologyManagers.push_back(hwlocTopologyManager.get());

  // Detecting Ascend
  #ifdef _HICR_USE_ASCEND_BACKEND_
      auto ascendTopologyManager = HiCR::backend::ascend::L1::TopologyManager::createDefault();
      topologyManagers.push_back(ascendTopologyManager.get());
  #endif

  // Creating HiCR Runtime
  auto runtime = HiCR::Runtime(instanceManager.get(), communicationManager.get(), memoryManager.get(), topologyManagers);

  // Registering tasks for the coordinator and the workers
  runtime.registerEntryPoint("Coordinator", [&]() { coordinatorEntryPointFc(runtime); });
  runtime.registerEntryPoint("Worker A",    [&]() { workerEntryPointFc(runtime, "A"); });
  runtime.registerEntryPoint("Worker B",    [&]() { workerEntryPointFc(runtime, "B"); });
  runtime.registerEntryPoint("Worker C",    [&]() { workerEntryPointFc(runtime, "C"); });

  // Initializing the HiCR runtime
  runtime.initialize();

  auto coordinator = runtime.getCurrentInstance();
  printf("Cooridnator id: %lu\n", coordinator->getHiCRInstance()->getId());

  // If the number of arguments passed is incorrect, abort execution and exit
  if (argc != 2)
  {
    fprintf(stderr, "Launch error. No machine model file provided\n");
    runtime.abort(-1);
  }

  // Parsing number of instances requested
  auto machineModelFile = std::string(argv[1]);

  // Loading machine model
  auto machineModel = loadMachineModelFromFile(machineModelFile);

  // If the machine model is empty, it's either erroneous or empty
  if (machineModel.empty())
  {
    fprintf(stderr, "Launch error. Machine model is erroneous or empty\n");
    runtime.abort(-1);
  }

  // Finally, deploying machine model
  runtime.deploy(machineModel, &isTopologyAcceptable);

  // Finalizing runtime
  runtime.finalize();

  printf("Coordinator Reached End Main\n");

  return 0;
}
