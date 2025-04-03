/*
 *   Copyright 2025 Huawei Technologies Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <mpi.h>
#include <hicr/backends/mpi/L1/instanceManager.hpp>
#include <hicr/backends/mpi/L1/memoryManager.hpp>
#include <hicr/backends/mpi/L1/communicationManager.hpp>
#include <hicr/backends/pthreads/L1/computeManager.hpp>
#include <hicr/backends/hwloc/L1/topologyManager.hpp>
#include "include/coordinator.hpp"
#include "include/worker.hpp"

int main(int argc, char **argv)
{
  // Initializing MPI
  int requested = MPI_THREAD_SERIALIZED;
  int provided;
  MPI_Init_thread(&argc, &argv, requested, &provided);
  if (provided < requested) fprintf(stderr, "Warning, this example may not work properly if MPI does not support (serialized) threaded access\n");

  // Creating MPI-based instance manager
  HiCR::backend::mpi::L1::InstanceManager im(MPI_COMM_WORLD);

  // Creating compute manager (responsible for executing the RPC)
  HiCR::backend::pthreads::L1::ComputeManager cpm;

  // Creating memory and communication managers (buffering and communication)
  HiCR::backend::mpi::L1::MemoryManager        mm;
  HiCR::backend::mpi::L1::CommunicationManager cm;

  // Creating HWloc topology object
  hwloc_topology_t topology;

  // Reserving memory for hwloc
  hwloc_topology_init(&topology);

  // Initializing hwloc (CPU) topology manager
  HiCR::backend::hwloc::L1::TopologyManager tm(&topology);

  // Gathering topology from the topology manager
  const auto t = tm.queryTopology();

  // Selecting first device
  auto d = *t.getDevices().begin();

  // Getting memory space list from device
  auto memSpaces = d->getMemorySpaceList();

  // Grabbing first memory space for buffering
  auto bufferMemorySpace = *memSpaces.begin();

  // Now getting compute resource list from device
  auto computeResources = d->getComputeResourceList();

  // Grabbing first compute resource for computing incoming RPCs
  auto computeResource = *computeResources.begin();

  // Creating RPC engine instance
  HiCR::frontend::RPCEngine rpcEngine(cm, im, mm, cpm, bufferMemorySpace, computeResource);

  // Initialize RPC engine
  rpcEngine.initialize();

  // Get the locally running instance
  auto myInstance = im.getCurrentInstance();

  // Bifurcating paths based on whether the instance is root or not
  if (myInstance->isRootInstance() == true) coordinatorFc(rpcEngine);
  if (myInstance->isRootInstance() == false) workerFc(rpcEngine);

  // Finalizing HiCR
  MPI_Finalize();

  return 0;
}
