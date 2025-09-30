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

#include <lpf/core.h>
#include <lpf/mpi.h>
#include <mpi.h>
#include <hicr/backends/lpf/memoryManager.hpp>
#include <hicr/backends/lpf/communicationManager.hpp>
#include <hicr/backends/hwloc/topologyManager.hpp>
#include "include/consumer.hpp"
#include "include/producer.hpp"

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

void spmd(lpf_t lpf, lpf_pid_t pid, lpf_pid_t nprocs, lpf_args_t args)
{
  // Capacity must be larger than zero
  int channelCapacity = (*(int *)args.input);
  if (channelCapacity == 0)
    if (pid == 0) fprintf(stderr, "Error: Cannot create channel with zero capacity.\n");

  // Initializing LPF
  CHECK(lpf_resize_message_queue(lpf, DEFAULT_MSGSLOTS));
  CHECK(lpf_resize_memory_register(lpf, DEFAULT_MEMSLOTS));
  CHECK(lpf_sync(lpf, LPF_SYNC_DEFAULT));

  // Creating HWloc topology object
  hwloc_topology_t topology;

  // Reserving memory for hwloc
  hwloc_topology_init(&topology);

  // Initializing hwloc-based host (CPU) topology manager
  HiCR::backend::hwloc::TopologyManager dm(&topology);

  // Asking backend to check the available devices
  const auto t = dm.queryTopology();

  // Getting first device found
  auto d = *t.getDevices().begin();

  // Obtaining memory spaces
  auto memSpaces = d->getMemorySpaceList();

  // Creating LPF memory and communication managers
  HiCR::backend::lpf::MemoryManager        m(lpf);
  HiCR::backend::lpf::CommunicationManager c(nprocs, pid, lpf);

  // Getting reference to the first memory space detected
  auto firstMemorySpace = *memSpaces.begin();
  // Calculating the number of producer processes
  size_t producerCount = nprocs - 1;

  // Rank 0 is consumer, the rest are producers
  if (pid == 0) consumerFc(m, m, c, c, firstMemorySpace, firstMemorySpace, channelCapacity, producerCount);
  if (pid >= 1) producerFc(m, m, c, c, firstMemorySpace, firstMemorySpace, channelCapacity, pid);
}

int main(int argc, char **argv)
{
  MPI_Init(&argc, &argv);

  int rank, size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  int capacity;
  if (rank == 0)
  {
    if (size < 2)
    {
      fprintf(stderr, "Error: Must use at least 2 processes\n");
      MPI_Abort(MPI_COMM_WORLD, -1);
    }
    // Checking arguments
    if (argc != 2)
    {
      fprintf(stderr, "Error: Must provide the channel capacity as argument.\n");
      MPI_Abort(MPI_COMM_WORLD, -1);
    }
    // For portability, only read STDIN from process 0
    capacity = atoi(argv[1]);
  }
  MPI_Bcast(&capacity, 1, MPI_INT, 0, MPI_COMM_WORLD);

  lpf_args_t args;
  memset(&args, 0, sizeof(lpf_args_t));
  args.input       = &capacity;
  args.input_size  = sizeof(int);
  args.output      = nullptr;
  args.output_size = 0;
  args.f_size      = 0;
  args.f_symbols   = nullptr;
  lpf_init_t init;
  CHECK(lpf_mpi_initialize_with_mpicomm(MPI_COMM_WORLD, &init));
  CHECK(lpf_hook(init, &spmd, args));
  CHECK(lpf_mpi_finalize(init));
  MPI_Finalize();

  return 0;
}
