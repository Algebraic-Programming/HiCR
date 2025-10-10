#include <cstdio>
#include <pthread.h>
#include <thread>

#include <hicr/backends/hwloc/topologyManager.hpp>
#include <hicr/backends/pthreads/instanceManager.hpp>

#include "../include/createInstance.hpp"

int main(int argc, char const *argv[])
{
  // Check argvs
  if (argc != 2) { HICR_THROW_RUNTIME("Pass the number of instances to create as argument"); }

  // Get instance count
  size_t instancesToCreate = std::atoi(argv[1]);

  // Determine the root instance id
  HiCR::Instance::instanceId_t rootInstanceId = pthread_self();

  // Declare entrypoint
  auto entrypoint = [&](HiCR::InstanceManager *parentInstanceManager) {
    // Cast to pthread instance manager
    auto p = dynamic_cast<HiCR::backend::pthreads::InstanceManager *>(parentInstanceManager);

    // Fail if the casting is not successful
    if (p == nullptr) { HICR_THROW_RUNTIME("Can not cast instance manager to a pthread-specific one"); }

    // Create instance manager
    auto createdInstanceManager = HiCR::backend::pthreads::InstanceManager(rootInstanceId, p->getEntrypoint());

    // Run worker function
    workerFc(createdInstanceManager);
  };

  // Create instance manager
  auto instanceManager = HiCR::backend::pthreads::InstanceManager(rootInstanceId, entrypoint);

  // Discover local topology
  auto topologyManager = HiCR::backend::hwloc::TopologyManager::createDefault();
  auto topology        = topologyManager->queryTopology();

  // Create the new instance
  createInstances(instanceManager, instancesToCreate, topology);

  // Finalize instance manager
  instanceManager.finalize();

  printf("Terminating execution\n");

  return 0;
}
