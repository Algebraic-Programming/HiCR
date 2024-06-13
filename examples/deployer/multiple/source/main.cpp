#include <hwloc.h>
#include <hicr/frontends/deployer/deployer.hpp>
#include <hicr/backends/mpi/L1/instanceManager.hpp>
#include <hicr/backends/mpi/L1/communicationManager.hpp>
#include <hicr/backends/mpi/L1/memoryManager.hpp>
#include <hicr/backends/host/hwloc/L1/topologyManager.hpp>

#ifdef _HICR_USE_ASCEND_BACKEND_
  #include <hicr/backends/ascend/L1/topologyManager.hpp>
#endif

#include "../../common.hpp"

// Coordinator entry point functions
void coordinatorEntryPointFc(HiCR::Deployer &deployer)
{
  // Getting instance manager from the deployer
  auto instanceManager = deployer.getInstanceManager();

  // Getting coordinator instance
  auto coordinator = deployer.getCurrentInstance();

  // Creating a welcome message
  std::string welcomeMsg = "Hello from the coordinator";

  // Buffer for data objects to transfer
  std::vector<std::shared_ptr<HiCR::deployer::DataObject>> dataObjects;

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
      coordinator->sendMessage(instance->getId(), &dataObjectId, sizeof(HiCR::deployer::DataObject::dataObjectId_t));
    }

  // Sending a message to myself just to test self-comunication
  size_t workerCount = instanceManager->getInstances().size() - 1;
  coordinator->sendMessage(coordinator->getHiCRInstance()->getId(), &workerCount, sizeof(workerCount));
  auto message = coordinator->recvMessage();
  printf("[Coordinator] Received worker count: %lu from myself\n", *(size_t *)message.data);

  // Testing release completion for all data objects
  bool allDataObjectsReleased = false;
  while (allDataObjectsReleased == false)
  {
    allDataObjectsReleased = true;
    for (auto &dataObject : dataObjects) allDataObjectsReleased &= dataObject->tryRelease();
  }

  printf("Coordinator Reached End Function\n");
}

// Worker entry point function
void workerEntryPointFc(HiCR::Deployer &deployer, const std::string &entryPointName)
{
  printf("Hello, I am instance Id %lu, executing entry point '%s'\n", deployer.getInstanceId(), entryPointName.c_str());

  // Getting my current worker instance
  auto currentInstance = deployer.getCurrentInstance();

  // Getting root (coordinator) instance id
  auto coordinatorInstanceId = deployer.getInstanceManager()->getRootInstanceId();

  // Get a message from the coordinator
  HiCR::deployer::Instance::message_t message;
  while (message.size == 0) message = currentInstance->recvMessageAsync();

  // Getting data object id from message
  const auto dataObjectId = *((HiCR::deployer::DataObject::dataObjectId_t *)message.data);

  // Printing data object id
  printf("[Worker %lu] Requesting data object id %u from coordinator.\n", deployer.getInstanceId(), dataObjectId);

  // Creating data object reference
  HiCR::deployer::DataObject srcDataObject(nullptr, 0, dataObjectId, coordinatorInstanceId, 0);

  // Getting data object from coordinator
  auto dataObject = currentInstance->getDataObject(srcDataObject);

  // Printing data object contents
  printf("[Worker %lu] Received message from coordinator: '%s'\n", deployer.getInstanceId(), (const char *)dataObject->getData());

  // Freeing up internal buffer
  dataObject->destroyBuffer();
};

int main(int argc, char *argv[])
{
  // Using MPI as instance, communication and memory manager for multiple instances
  auto instanceManager      = HiCR::backend::mpi::L1::InstanceManager::createDefault(&argc, &argv);
  auto communicationManager = std::make_unique<HiCR::backend::mpi::L1::CommunicationManager>();
  auto memoryManager        = std::make_unique<HiCR::backend::mpi::L1::MemoryManager>();

  // Using HWLoc and Ascend (if configured) as topology managers
  std::vector<HiCR::L1::TopologyManager *> topologyManagers;
  hwloc_topology_t                         topology;
  hwloc_topology_init(&topology);
  auto hwlocTopologyManager = std::make_unique<HiCR::backend::host::hwloc::L1::TopologyManager>(&topology);
  topologyManagers.push_back(hwlocTopologyManager.get());

// Detecting Ascend
#ifdef _HICR_USE_ASCEND_BACKEND_
  auto ascendTopologyManager = HiCR::backend::ascend::L1::TopologyManager::createDefault();
  topologyManagers.push_back(ascendTopologyManager.get());
#endif

  // Creating HiCR Deployer
  auto deployer = HiCR::Deployer(instanceManager.get(), communicationManager.get(), memoryManager.get(), topologyManagers);

  // Registering tasks for the coordinator and the workers
  deployer.registerEntryPoint("Coordinator", [&]() { coordinatorEntryPointFc(deployer); });
  deployer.registerEntryPoint("Worker A", [&]() { workerEntryPointFc(deployer, "A"); });
  deployer.registerEntryPoint("Worker B", [&]() { workerEntryPointFc(deployer, "B"); });
  deployer.registerEntryPoint("Worker C", [&]() { workerEntryPointFc(deployer, "C"); });

  // Initializing the HiCR deployer
  deployer.initialize();

  // If the number of arguments passed is incorrect, abort execution and exit
  if (argc != 2)
  {
    fprintf(stderr, "Launch error. No machine model file provided\n");
    deployer.abort(-1);
  }

  // Parsing number of instances requested
  auto machineModelFile = std::string(argv[1]);

  // Loading machine model
  auto machineModel = loadMachineModelFromFile(machineModelFile);

  // If the machine model is empty, it's either erroneous or empty
  if (machineModel.empty())
  {
    fprintf(stderr, "Launch error. Machine model is erroneous or empty\n");
    deployer.abort(-1);
  }

  // Finally, deploying machine model
  deployer.deploy(machineModel, &isTopologyAcceptable);

  // Finalizing deployer
  deployer.finalize();

  printf("Coordinator Reached End Main\n");

  return 0;
}
