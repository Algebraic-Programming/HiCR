#include "include/common.hpp"
#include "include/coordinator.hpp"
#include "include/worker.hpp"

int main(int argc, char *argv[])
{
  // Getting instance manager from the HiCR initialization
  auto instanceManager = HiCR::initialize(&argc, &argv);

  // Get the locally running instance
  auto myInstance = instanceManager->getCurrentInstance();

  // If the number of arguments passed is incorrect, abort execution and exit
  if (argc != 2)
  {
    if (myInstance->isRootInstance()) fprintf(stderr, "Launch error. No machine model file provided\n");
    HiCR::finalize();
    return -1;
  }

  // Parsing number of instances requested
  auto machineModelFile = std::string(argv[1]);

  // Bifurcating paths based on whether the instance is root or not
  if (myInstance->isRootInstance() == true) coordinatorFc(instanceManager, machineModelFile);
  if (myInstance->isRootInstance() == false) workerFc(instanceManager);

  // Finalizing HiCR
  HiCR::finalize();

  return 0;
}
