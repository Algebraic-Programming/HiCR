#include <hicr/frontends/runtime/runtime.hpp>
#include "../../common.hpp"

// Worker entry point functions
void entryPointFc(HiCR::Runtime &runtime, const std::string &entryPointName)
{
  printf("Hello, I am the coordinator itself\n");
};

int main(int argc, char *argv[])
{

  To-DO: do the same as in multiple but here with host, hwloc and pthreads only
  // Creating HiCR Runtime
  HiCR::Runtime runtime(&argc, &argv);

  // Registering tasks for the workers
  runtime.registerEntryPoint("A", [&]() { entryPointFc(runtime, "A"); });

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

  // Finally, deploying machine model
  runtime.deploy(machineModel, &isTopologyAcceptable);

  // Finalizing runtime
  runtime.finalize();

  return 0;
}
