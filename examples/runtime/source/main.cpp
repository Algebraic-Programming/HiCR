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

   // Printinf message
   printf("[Worker %lu] Received message from coordinator: '%s'\n", HiCR::Runtime::getInstanceId(), (const char*) message.first);
 };

int main(int argc, char *argv[])
{
  // Registering tasks for the workers
  HiCR::Runtime::registerEntryPoint("Task A",   [&]() { entryPointFc("A"); });
  HiCR::Runtime::registerEntryPoint("Task B",   [&]() { entryPointFc("B"); });
  HiCR::Runtime::registerEntryPoint("Task C",   [&]() { entryPointFc("C"); });

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
  
  // Sending message to all the workers
  for (auto& worker : coordinator->getWorkers()) coordinator->sendMessage(worker, welcomeMsg.data(), welcomeMsg.size());

  // Finalizing runtime
  HiCR::Runtime::finalize();

  return 0;
}
