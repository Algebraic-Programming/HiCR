/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file worker.hpp
 * @brief Provides a definition for the Runtime Coordinator class.
 * @author S. M. Martin
 * @date 5/2/2024
 */

#pragma once

#include <memory>
#include <hicr/definitions.hpp>
#include <hicr/exceptions.hpp>
#include "channel.hpp"
#include "instance.hpp"

namespace HiCR
{

namespace runtime
{

/**
 * Defines the coordinator class, which represents a self-contained instance of HiCR with access to compute and memory resources.
 *
 * There can be only a single coordinator per deployment and its job is to supervise the worker instances
 */
class Coordinator final : public runtime::Instance
{
  public:

  /** 
  * Storage for worker data as required by the coordinator for issuing RPCs and sending messages
  */
  struct worker_t
  {
    /**
     * Initial entry point function name being executed by the worker
     */ 
    const std::string entryPoint;

    /**
     * Internal HiCR instance class
     */ 
    const std::shared_ptr<HiCR::L0::Instance> hicrInstance;

    /**
     * Consumer channel to receive RPCs from the coordinator instance
     */ 
    std::shared_ptr<runtime::Channel> rpcChannel;
  };

  Coordinator() = delete;
  Coordinator(HiCR::L1::InstanceManager& instanceManager,
          HiCR::L1::CommunicationManager& communicationManager,
          HiCR::L1::MemoryManager& memoryManager,
          std::vector<HiCR::L1::TopologyManager*>& topologyManagers,
          HiCR::MachineModel& machineModel
        ) : runtime::Instance(instanceManager, communicationManager, memoryManager, topologyManagers, machineModel) {}
  ~Coordinator() = default;

  void initialize()
  { }

  /**
   * Deploys the requested machine model and uses the a user-provided acceptance criteria function to evaluate whether the allotted resources satisfy the request
   *
   * @param[in] requests A vector of instance requests, expressing the requested system's machine model and the tasks that each instance needs to run
   * @param[in] acceptanceCriteriaFc A user-given function that compares the requested topology for a given instance and the one obtained to decide whether it meets the user requirements
   */
  void deploy(std::vector<HiCR::MachineModel::request_t> &requests, HiCR::MachineModel::topologyAcceptanceCriteriaFc_t acceptanceCriteriaFc, int argc, char *argv[])
  {
    // Execute requests by finding or creating an instance that matches their topology requirements
    try
    {
      _machineModel->deploy(requests, acceptanceCriteriaFc, argc, argv);
    }
    catch (const std::exception &e)
    {
      fprintf(stderr, "Error while executing requests. Reason: '%s'\n", e.what());
      _abort(-1);
    }

    // Running the assigned task id in the corresponding instance and registering it as worker
    for (auto &r : requests)
      for (auto &in : r.instances)
      {
        // Creating worker instance
        auto worker = worker_t { .entryPoint = r.taskName, .hicrInstance = in };

        // Adding worker to the set
        _workers.push_back(worker);
      } 

    // Launching channel creation routine for every worker
    for (auto &w : _workers) _instanceManager->launchRPC(*w.hicrInstance, "__initializeRPCChannels");

    // Initializing RPC channels
    initializeRPCChannels();

    // Launching worker's entry point function     
    for (auto &w : _workers) _instanceManager->launchRPC(*w.hicrInstance, w.entryPoint); 
  }

  void finalize()
  {
    // Launching finalization RPC
    for (auto &w : _workers) _instanceManager->launchRPC(*w.hicrInstance, "__finalize");

    // Waiting for return ack
    for (auto &w : _workers) _instanceManager->getReturnValue(*w.hicrInstance);

    _instanceManager->finalize();
  }

  private:

  void initializeRPCChannels()
  {
    ////////// Creating producer channels to send variable sized RPCs fto the workers

      // Accessing first topology manager detected
      auto& tm = *_topologyManagers[0];

      // Gathering topology from the topology manager
      const auto t = tm.queryTopology();

      // Selecting first device
      auto d = *t.getDevices().begin();

      // Getting memory space list from device
      auto memSpaces = d->getMemorySpaceList();

      // Grabbing first memory space for buffering
      auto bufferMemorySpace = *memSpaces.begin();

      // Getting required buffer size for coordination buffers
      auto coordinationBufferSize = HiCR::channel::variableSize::Base::getCoordinationBufferSize();

      // Storage for dedicated channels
      std::vector<std::pair<HiCR::L0::Instance::instanceId_t, std::shared_ptr<HiCR::L0::LocalMemorySlot>>> coordinationBufferMessageSizesVector;
      std::vector<std::pair<HiCR::L0::Instance::instanceId_t, std::shared_ptr<HiCR::L0::LocalMemorySlot>>> coordinationBufferMessagePayloadsVector;
      std::vector<std::shared_ptr<HiCR::L0::LocalMemorySlot>> sizeInfoBufferMemorySlotVector;
      
      // Create coordinator buffers for each of the channels (one per worker)
      for (size_t i = 0; i < _workers.size(); i++)
      {
        // Allocating coordination buffers
        auto coordinationBufferMessageSizes    = _memoryManager->allocateLocalMemorySlot(bufferMemorySpace, coordinationBufferSize);
        auto coordinationBufferMessagePayloads = _memoryManager->allocateLocalMemorySlot(bufferMemorySpace, coordinationBufferSize);
        auto sizeInfoBufferMemorySlot          = _memoryManager->allocateLocalMemorySlot(bufferMemorySpace, sizeof(size_t));

        // Initializing coordination buffers
        HiCR::channel::variableSize::Base::initializeCoordinationBuffer(coordinationBufferMessageSizes);
        HiCR::channel::variableSize::Base::initializeCoordinationBuffer(coordinationBufferMessagePayloads);

        // Getting worker instance id
        const auto workerInstanceId = _workers[i].hicrInstance->getId();

        // Adding buffers to their respective vectors
        coordinationBufferMessageSizesVector.push_back({workerInstanceId, coordinationBufferMessageSizes});
        coordinationBufferMessagePayloadsVector.push_back({workerInstanceId, coordinationBufferMessagePayloads});
        sizeInfoBufferMemorySlotVector.push_back(sizeInfoBufferMemorySlot);
      }

      // Exchanging local memory slots to become global for them to be used by the remote end
      _communicationManager->exchangeGlobalMemorySlots(_HICR_RUNTIME_CHANNEL_WORKER_SIZES_BUFFER_TAG,                      {});
      _communicationManager->fence(_HICR_RUNTIME_CHANNEL_WORKER_SIZES_BUFFER_TAG);

      _communicationManager->exchangeGlobalMemorySlots(_HICR_RUNTIME_CHANNEL_WORKER_PAYLOAD_BUFFER_TAG,                    {});
      _communicationManager->fence(_HICR_RUNTIME_CHANNEL_WORKER_PAYLOAD_BUFFER_TAG);

      _communicationManager->exchangeGlobalMemorySlots(_HICR_RUNTIME_CHANNEL_WORKER_COORDINATION_BUFFER_SIZES_TAG,         {});
      _communicationManager->fence(_HICR_RUNTIME_CHANNEL_WORKER_COORDINATION_BUFFER_SIZES_TAG);

      _communicationManager->exchangeGlobalMemorySlots(_HICR_RUNTIME_CHANNEL_WORKER_COORDINATION_BUFFER_PAYLOADS_TAG,      {});
      _communicationManager->fence(_HICR_RUNTIME_CHANNEL_WORKER_COORDINATION_BUFFER_PAYLOADS_TAG);

      _communicationManager->exchangeGlobalMemorySlots(_HICR_RUNTIME_CHANNEL_COORDINATOR_COORDINATION_BUFFER_SIZES_TAG,    coordinationBufferMessageSizesVector);
      _communicationManager->fence(_HICR_RUNTIME_CHANNEL_COORDINATOR_COORDINATION_BUFFER_SIZES_TAG);
      
      _communicationManager->exchangeGlobalMemorySlots(_HICR_RUNTIME_CHANNEL_COORDINATOR_COORDINATION_BUFFER_PAYLOADS_TAG, coordinationBufferMessagePayloadsVector);
      _communicationManager->fence(_HICR_RUNTIME_CHANNEL_COORDINATOR_COORDINATION_BUFFER_PAYLOADS_TAG); 

      printf("Memory slots exchanged\n");

      // Creating producer channels
      for (size_t i = 0; i < _workers.size(); i++)
      {
        // Getting worker
        const auto& worker = _workers[i];

        // Getting worker instance id
        const auto workerInstanceId = worker.hicrInstance->getId();

        // Obtaining the globally exchanged memory slots
        auto workerMessageSizesBuffer            = _communicationManager->getGlobalMemorySlot(_HICR_RUNTIME_CHANNEL_WORKER_SIZES_BUFFER_TAG,                      workerInstanceId);
        auto workerMessagePayloadBuffer          = _communicationManager->getGlobalMemorySlot(_HICR_RUNTIME_CHANNEL_WORKER_PAYLOAD_BUFFER_TAG,                    workerInstanceId);
        auto coordinatorSizesCoordinatorBuffer   = _communicationManager->getGlobalMemorySlot(_HICR_RUNTIME_CHANNEL_COORDINATOR_COORDINATION_BUFFER_SIZES_TAG,    workerInstanceId);
        auto coordinatorPayloadCoordinatorBuffer = _communicationManager->getGlobalMemorySlot(_HICR_RUNTIME_CHANNEL_COORDINATOR_COORDINATION_BUFFER_PAYLOADS_TAG, workerInstanceId);

        // Creating channel
        auto producerChannel = HiCR::channel::variableSize::SPSC::Producer(
          *_communicationManager,
          sizeInfoBufferMemorySlotVector[i],
          workerMessagePayloadBuffer,
          workerMessageSizesBuffer,
          coordinationBufferMessageSizesVector[i].second,
          coordinationBufferMessagePayloadsVector[i].second,
          coordinatorSizesCoordinatorBuffer,
          coordinatorPayloadCoordinatorBuffer,
          _HICR_RUNTIME_CHANNEL_PAYLOAD_CAPACITY,
          sizeof(uint8_t),
          _HICR_RUNTIME_CHANNEL_COUNT_CAPACITY
        );

        // Sending initial message
        std::string message = "Hello from the coordinator";
        auto messageSendSlot = _memoryManager->registerLocalMemorySlot(bufferMemorySpace, message.data(), message.size());
        producerChannel.push(messageSendSlot);
      }
  }

 /**
  * Storage for the deployed workers. This object is only mantained and usable by the coordinator
  */
  std::vector<worker_t> _workers;
};

} // namespace runtime

} // namespace HiCR
