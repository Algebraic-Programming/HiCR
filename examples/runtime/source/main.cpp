#include <frontends/runtime/runtime.hpp>
#include "include/machineModel.hpp"

// Worker entry point functions
void entryPointFc(const std::string &entryPointName)
{
  printf("Hello, I am worker %lu, executing entry point '%s'\n", HiCR::Runtime::getInstanceId(), entryPointName.c_str());

  // Getting my current worker instance
  auto currentWorker = HiCR::Runtime::getWorkerInstance();

  // Getting message from coordinator
  auto message = currentWorker->recvMessage();  

  // Getting data object id from message
  const auto dataObjectId = *((HiCR::runtime::DataObject::dataObjectId_t*) message.first);

  // Printing data object id
  printf("[Worker %lu] Requesting data object id %u from coordinator.\n", HiCR::Runtime::getInstanceId(), dataObjectId);

  // Getting data object from coordinator
  auto dataObject = currentWorker->getDataObject(dataObjectId);

  // Printing data object contents
  printf("[Worker %lu] Received message from coordinator: '%s'\n", HiCR::Runtime::getInstanceId(), (const char*) dataObject->getData());

  // Freeing up internal buffer
  dataObject->destroyBuffer();
};

int main(int argc, char *argv[])
{
  // Registering tasks for the workers
  HiCR::Runtime::registerEntryPoint("A", [&]() { entryPointFc("A"); });
  HiCR::Runtime::registerEntryPoint("B", [&]() { entryPointFc("B"); });
  HiCR::Runtime::registerEntryPoint("C", [&]() { entryPointFc("C"); });

  // Initializing the HiCR singleton
  HiCR::Runtime::initialize(&argc, &argv);
  
  // If the number of arguments passed is incorrect, abort execution and exit
  if (argc != 2)
  {
    fprintf(stderr, "Launch error. No machine model file provided\n");
    HiCR::Runtime::abort(-1);
  }

  // Parsing number of instances requested
  auto machineModelFile = std::string(argv[1]);

  // Loading machine model
  auto machineModel = loadMachineModelFromFile(machineModelFile);

  // Finally, deploying machine model
  HiCR::Runtime::deploy(machineModel, &isTopologyAcceptable);
  
  // Getting coordinator instance
  auto coordinator = HiCR::Runtime::getCoordinatorInstance();

  // Creating a welcome message
  std::string welcomeMsg = "Hello from the coordinator";

  // Buffer for data objects to transfer
  std::vector<std::shared_ptr<HiCR::runtime::DataObject>> dataObjects;

  // Sending message to all the workers
  for (auto& worker : coordinator->getWorkers()) 
  {
    // Creating data object with welcome message
    auto dataObject = coordinator->createDataObject(welcomeMsg.data(), welcomeMsg.size()+1);

    // Getting data object identifier
    auto dataObjectId = dataObject->getId();

    // Publishing data object
    dataObject->publish();

    // Adding data object to the vector
    dataObjects.push_back(dataObject);

    // Sending message with only the data object identifier
    coordinator->sendMessage(worker, &dataObjectId, sizeof(HiCR::runtime::DataObject::dataObjectId_t));
  }

  // Testing release completion for all data objects
  bool allDataObjectsReleased = false;
  while (allDataObjectsReleased == false)
  {
    allDataObjectsReleased = true;
    for (auto& dataObject : dataObjects)
     allDataObjectsReleased &= dataObject->release();
  }

  // Finalizing runtime
  HiCR::Runtime::finalize();

  return 0;
}
