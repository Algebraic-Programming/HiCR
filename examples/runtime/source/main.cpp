#include <frontends/runtime/runtime.hpp>
#include "include/machineModel.hpp"

// Worker entry point functions
void entryPointFc(const std::string &entryPointName) { printf("Hello, I am worker %lu, executing entry point '%s'\n", HiCR::Runtime::getInstanceId(), entryPointName.c_str());  };

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
    HiCR::Runtime::finalize();
    return -1;
  }

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
  
  // Finalizing runtime
  HiCR::Runtime::finalize();

  return 0;
}
