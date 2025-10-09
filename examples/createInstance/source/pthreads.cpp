#include <cstdio>
#include <pthread.h>

#include <hicr/backends/hwloc/topologyManager.hpp>
#include <hicr/backends/pthreads/instanceManager.hpp>

#include "../include/createInstance.hpp"

int main(int argc, char const *argv[])
{
  // Check argvs
  if (argc != 2) { HICR_THROW_RUNTIME("Pass the instance count as argument"); }

  // Get instance count
  auto instanceCount = std::atoi(argv[1]);

  // Determine the root instance id
  HiCR::Instance::instanceId_t rootInstanceId = pthread_self();

  // Create barrier
  pthread_barrier_t barrier{};
  pthread_barrier_init(&barrier, nullptr, instanceCount);

  // Declare entrypoint
  auto entrypoint = [&](HiCR::backend::pthreads::InstanceManager *creatorIm) {
    auto im = HiCR::backend::pthreads::InstanceManager(rootInstanceId, creatorIm->getEntrypoint());
    printf("[Instance %lu] Hello World\n", im.getCurrentInstance()->getId());
    pthread_barrier_wait(&barrier);
    printf("[Instance %lu] fininshing execution\n", im.getCurrentInstance()->getId());
  };

  // Create instance manager
  auto im = HiCR::backend::pthreads::InstanceManager(rootInstanceId, entrypoint);

  // Discover local topology
  auto tm = HiCR::backend::hwloc::TopologyManager::createDefault();
  auto t  = tm->queryTopology();

  // Create the new instance
  createInstances(im, instanceCount, t);

  // Wait barrier
  pthread_barrier_wait(&barrier);

  printf("Terminating execution\n");

  // Finalize instance manager
  im.finalize();

  return 0;
}
