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

#include <iostream>
#include <lpf/core.h>
#include <lpf/mpi.h>
#include <mpi.h>

#include <hicr/backends/mpi/instanceManager.hpp>
#include <hicr/backends/lpf/memoryManager.hpp>
#include <hicr/backends/lpf/communicationManager.hpp>
#include <hicr/backends/hwloc/topologyManager.hpp>
#include "include/remoteMemcpy.hpp"

// flag needed when using MPI to launch
const int LPF_MPI_AUTO_INITIALIZE = 0;

/**
 * #DEFAULT_MEMSLOTS The memory slots used by LPF
 * in lpf_resize_memory_register . This value is currently
 * guessed as sufficiently large for a program
 */
#define DEFAULT_MEMSLOTS 100

/**
 * #DEFAULT_MSGSLOTS The message slots used by LPF
 * in lpf_resize_message_queue . This value is currently
 * guessed as sufficiently large for a program
 */
#define DEFAULT_MSGSLOTS 100

// Global pointer to the
HiCR::InstanceManager *instanceManager;

void spmd(lpf_t lpf, lpf_pid_t pid, lpf_pid_t nprocs, lpf_args_t args)
{
  // Initializing LPF
  CHECK(lpf_resize_message_queue(lpf, DEFAULT_MSGSLOTS));
  CHECK(lpf_resize_memory_register(lpf, DEFAULT_MEMSLOTS));
  CHECK(lpf_sync(lpf, LPF_SYNC_DEFAULT));

  // Creating HWloc topology object
  hwloc_topology_t topology;

  // Reserving memory for hwloc
  hwloc_topology_init(&topology);

  // Initializing host (CPU) topology manager
  HiCR::backend::hwloc::TopologyManager tm(&topology);

  // Creating memory and communication managers
  HiCR::backend::lpf::MemoryManager        mm(lpf);
  HiCR::backend::lpf::CommunicationManager cc(nprocs, pid, lpf);

  // Running the remote memcpy example
  remoteMemcpy(instanceManager, &tm, &mm, &cc);
}

int main(int argc, char **argv)
{
  // Initializing instance manager
  auto im         = HiCR::backend::mpi::InstanceManager::createDefault(&argc, &argv);
  instanceManager = im.get();

  lpf_init_t init;
  lpf_args_t args;

  CHECK(lpf_mpi_initialize_with_mpicomm(MPI_COMM_WORLD, &init));
  CHECK(lpf_hook(init, &spmd, args));
  CHECK(lpf_mpi_finalize(init));

  // Finalizing instance manager
  im->finalize();
  return 0;
}
