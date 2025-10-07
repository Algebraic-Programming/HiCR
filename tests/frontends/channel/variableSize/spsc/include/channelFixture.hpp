#pragma once

#include <gtest/gtest.h>
#include <mpi.h>

#include <hicr/backends/hwloc/topologyManager.hpp>
#include <hicr/backends/mpi/communicationManager.hpp>
#include <hicr/backends/mpi/instanceManager.hpp>
#include <hicr/backends/mpi/memoryManager.hpp>
#include <hicr/backends/pthreads/computeManager.hpp>
#include <hicr/frontends/RPCEngine/RPCEngine.hpp>

class ChannelFixture : public ::testing::Test
{
  public:

  ChannelFixture()
    : ::testing::Test()
  {
    _mpiInstanceManager      = std::make_unique<HiCR::backend::mpi::InstanceManager>(MPI_COMM_WORLD);
    _mpiCommunicationManager = std::make_unique<HiCR::backend::mpi::CommunicationManager>(MPI_COMM_WORLD);
    _mpiMemoryManager        = std::make_unique<HiCR::backend::mpi::MemoryManager>();
    _pthreadsComputeManager  = std::make_unique<HiCR::backend::pthreads::ComputeManager>();

    _hwlocTopologyManager = HiCR::backend::hwloc::TopologyManager::createDefault();

    _topology = _hwlocTopologyManager->queryTopology();
  }

  std::unique_ptr<HiCR::CommunicationManager> _mpiCommunicationManager;
  std::unique_ptr<HiCR::InstanceManager>      _mpiInstanceManager;
  std::unique_ptr<HiCR::MemoryManager>        _mpiMemoryManager;
  std::unique_ptr<HiCR::TopologyManager>      _hwlocTopologyManager;

  std::unique_ptr<HiCR::backend::pthreads::ComputeManager> _pthreadsComputeManager;

  HiCR::Topology _topology;
};