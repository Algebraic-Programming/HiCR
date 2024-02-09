#include <frontends/runtime/runtime.hpp>
#include "include/machineModel.hpp"

// Worker task function
void taskFc(const std::string &taskName) { printf("Hello, I am worker %lu, executing task '%s'\n", HiCR::Runtime::getInstanceId(), taskName.c_str()); };

int main(int argc, char *argv[])
{
  HiCR::Runtime::initialize(&argc, &argv);
  
  // If the number of arguments passed is incorrect, abort execution and exit
  if (argc != 2)
  {
    if (HiCR::Runtime::isCoordinator()) fprintf(stderr, "Launch error. No machine model file provided\n");
    HiCR::Runtime::finalize();
    return -1;
  }

  // If worker, register tasks and start listening
  if (HiCR::Runtime::isWorker())
  {
    // Registering tasks for the forkers
    HiCR::Runtime::addTask("Task A",   [&]() { taskFc("A"); });
    HiCR::Runtime::addTask("Task B",   [&]() { taskFc("B"); });
    HiCR::Runtime::addTask("Task C",   [&]() { taskFc("C"); });

    // Start listening
    HiCR::Runtime::listen();
  } 

  // If coordinator, read, parse and deploy configuration file
  if (HiCR::Runtime::isCoordinator())
  {
    // Parsing number of instances requested
    auto machineModelFile = std::string(argv[1]);

    // Reading from machine model file
    std::string machineModelRaw;
    auto status = loadStringFromFile(machineModelRaw, machineModelFile);
    if (status == false)
    {
      fprintf(stderr, "could not read from machine model file: '%s'\n", machineModelFile.c_str());
      HiCR::Runtime::abort(-1);
    }

    // Parsing received machine model file
    nlohmann::json machineModelJson;
    try
    {
      machineModelJson = nlohmann::json::parse(machineModelRaw);
    }
    catch (const std::exception &e)
    {
      fprintf(stderr, "could not parse JSON from machine model file: '%s'. Reason: '%s'\n", machineModelFile.c_str(), e.what());
      HiCR::Runtime::abort(-1);
    }

    // Parsing the machine model into a request vector. Here the vector implies ordering, which allows the user specify which instances need to be allocated first
    std::vector<HiCR::MachineModel::request_t> requests;
    try
    {
      requests = parseMachineModel(machineModelJson);
    }
    catch (const std::exception &e)
    {
      fprintf(stderr, "Error while parsing the machine model. Reason: '%s'\n", e.what());
      HiCR::Runtime::abort(-1);
    }

    // Finally, deploying machine model
    HiCR::Runtime::deploy(requests, &isTopologyAcceptable);
  }
  
  // Finalizing runtime
  HiCR::Runtime::finalize();

  return 0;
}
