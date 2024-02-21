#include <frontends/runtime/runtime.hpp>
#include "include/machineModel.hpp"

// Worker entry point functions
void entryPointFc(HiCR::Runtime& runtime, const std::string &entryPointName)
{
  printf("Hello, I am worker %lu, executing entry point '%s'\n", runtime.getInstanceId(), entryPointName.c_str());

  // Getting my current worker instance
  auto currentWorker = runtime.getWorkerInstance();

  // Getting message from coordinator
  const auto coordinatorInstanceId = runtime.getCoordinatorInstanceId();
  auto message = currentWorker->recvMessage(coordinatorInstanceId);  

  // Getting data object id from message
  const auto dataObjectId = *((HiCR::runtime::DataObject::dataObjectId_t*) message.first);

  // Printing data object id
  printf("[Worker %lu] Requesting data object id %u from coordinator.\n", runtime.getInstanceId(), dataObjectId);

  // Getting data object from coordinator
  auto dataObject = currentWorker->getDataObject(dataObjectId);

  // Printing data object contents
  printf("[Worker %lu] Received message from coordinator: '%s'\n", runtime.getInstanceId(), (const char*) dataObject->getData());

  // Freeing up internal buffer
  dataObject->destroyBuffer();
};

int main(int argc, char *argv[])
{
  // Creating HiCR Runtime
  HiCR::Runtime runtime(&argc, &argv);

  // Registering tasks for the workers
  runtime.registerEntryPoint("A", [&]() { entryPointFc(runtime, "A"); });
  runtime.registerEntryPoint("B", [&]() { entryPointFc(runtime, "B"); });
  runtime.registerEntryPoint("C", [&]() { entryPointFc(runtime, "C"); });

  // Initializing the HiCR runtime
  runtime.initialize();
  
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
  
  // Getting coordinator instance
  auto coordinator = runtime.getCoordinatorInstance();

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
    coordinator->sendMessage(worker.hicrInstance->getId(), &dataObjectId, sizeof(HiCR::runtime::DataObject::dataObjectId_t));
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
  runtime.finalize();

  return 0;
}
