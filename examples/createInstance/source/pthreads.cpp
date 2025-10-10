#include <cstdio>
#include <pthread.h>
#include <thread>

#include <hicr/backends/hwloc/topologyManager.hpp>
#include <hicr/backends/pthreads/instanceManager.hpp>
#include <hicr/backends/pthreads/instancePool.hpp>

#include "../include/createInstance.hpp"

int main(int argc, char const *argv[])
{
  // Check argvs
  if (argc != 3) { HICR_THROW_RUNTIME("Pass the instance count as argument"); }

  // Get instance count
  size_t instanceCount    = std::atoi(argv[1]);
  size_t instanceToCreate = std::atoi(argv[2]);

  // Determine the root instance id
  HiCR::Instance::instanceId_t rootInstanceId = pthread_self();

  // Create Instance pool
  HiCR::backend::pthreads::InstancePool instancePool(0);

  // Declare entrypoint
  auto entrypoint = [&](HiCR::backend::pthreads::InstanceManager *creatorIm) {
    auto im = HiCR::backend::pthreads::InstanceManager(rootInstanceId, creatorIm->getEntrypoint(), instancePool);
    printf("[Instance %lu] Hello World\n", im.getCurrentInstance()->getId());
  };

  // Define initial threads function
  auto workload = [&]() {
    // Create instance manager
    auto im = HiCR::backend::pthreads::InstanceManager(rootInstanceId, entrypoint, instancePool);

    // Detect already started instances
    im.detectInstances(instanceCount);

    // Discover local topology
    auto tm = HiCR::backend::hwloc::TopologyManager::createDefault();
    auto t  = tm->queryTopology();

    // Create the new instance
    createInstances(im, instanceToCreate, t);

    // Finalize instance manager
    im.finalize();
  };

  std::vector<std::unique_ptr<std::thread>> initialThreads;
  for (size_t i = 0; i < instanceCount - 1; i++)
  {
    // Create thread running the workload
    auto thread = std::make_unique<std::thread>(workload);

    // Add to the vector of initial threads
    initialThreads.push_back(std::move(thread));
  }

  // Run the workload
  workload();

  // Wait for all the threads to join
  for (auto &thread : initialThreads) { thread->join(); }

  printf("Terminating execution\n");

  return 0;
}
