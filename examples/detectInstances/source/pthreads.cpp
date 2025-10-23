#include <cstdio>
#include <pthread.h>
#include <thread>

#include <hicr/backends/pthreads/instanceManager.hpp>

void launcher(int argc, char const *argv[], HiCR::backend::pthreads::Core *core)
{
  // Create instance manager
  auto instanceManager = HiCR::backend::pthreads::InstanceManager(*core);

  // Get current instance
  auto currentInstance = instanceManager.getCurrentInstance();

  // Check if root
  if (currentInstance->isRootInstance()) { printf("[Instance %lu] is root\n", currentInstance->getId()); }

  // Get detected instances
  auto instances = instanceManager.getInstances();
  printf("[Instance %lu] Detected %lu instances\n", currentInstance->getId(), instances.size());

  // Finalize instance manager
  instanceManager.finalize();
}

int main(int argc, char const *argv[])
{
  // Check argvs
  if (argc != 2) { HICR_THROW_RUNTIME("Pass the number of instances to create as argument"); }

  // Get instance count
  size_t instancesToDetect = std::atoi(argv[1]);

  // Create core with the desired fence count
  auto core = HiCR::backend::pthreads::Core(instancesToDetect);

  // Create N-1 threads that runs the launcher
  auto threads = std::vector<std::thread>();
  for (size_t i = 0; i < instancesToDetect - 1; i++)
  {
    threads.emplace_back([&]() { launcher(argc, argv, &core); });
  }

  // Execute launcher ourselves
  launcher(argc, argv, &core);

  // Wait for threads to terminate
  for (auto &t : threads) { t.join(); }

  return 0;
}
