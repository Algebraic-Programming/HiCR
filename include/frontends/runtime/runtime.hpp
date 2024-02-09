
/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file runtime.hpp
 * @brief This file implements the runtime class
 * @author S. M Martin
 * @date 31/01/2024
 */

#pragma once

#include <vector>
#include <hicr/L1/instanceManager.hpp>
#include <hicr/L1/memoryManager.hpp>
#include <hicr/L1/communicationManager.hpp>
#include <hicr/L1/topologyManager.hpp>
#include <frontends/machineModel/machineModel.hpp>
#include "worker.hpp"

#ifdef _HICR_USE_MPI_BACKEND_
  #include <backends/mpi/L1/instanceManager.hpp>
  #include <backends/mpi/L1/communicationManager.hpp>
  #include <backends/mpi/L1/memoryManager.hpp>
  #include <frontends/channel/variableSize/spsc/consumer.hpp>
  #include <frontends/channel/variableSize/spsc/producer.hpp>

  #define _HICR_RUNTIME_CHANNEL_PAYLOAD_CAPACITY 1048576
  #define _HICR_RUNTIME_CHANNEL_COUNT_CAPACITY 1024
  #define _HICR_RUNTIME_CHANNEL_BASE_TAG 0xF0000000
  #define _HICR_RUNTIME_CHANNEL_WORKER_SIZES_BUFFER_TAG                      _HICR_RUNTIME_CHANNEL_BASE_TAG + 0
  #define _HICR_RUNTIME_CHANNEL_WORKER_PAYLOAD_BUFFER_TAG                    _HICR_RUNTIME_CHANNEL_BASE_TAG + 1
  #define _HICR_RUNTIME_CHANNEL_WORKER_COORDINATION_BUFFER_SIZES_TAG         _HICR_RUNTIME_CHANNEL_BASE_TAG + 3
  #define _HICR_RUNTIME_CHANNEL_WORKER_COORDINATION_BUFFER_PAYLOADS_TAG      _HICR_RUNTIME_CHANNEL_BASE_TAG + 4
  #define _HICR_RUNTIME_CHANNEL_COORDINATOR_COORDINATION_BUFFER_SIZES_TAG    _HICR_RUNTIME_CHANNEL_BASE_TAG + 5
  #define _HICR_RUNTIME_CHANNEL_COORDINATOR_COORDINATION_BUFFER_PAYLOADS_TAG _HICR_RUNTIME_CHANNEL_BASE_TAG + 6
#endif

#ifdef _HICR_USE_YUANRONG_BACKEND_
  #include <backends/yuanrong/L1/instanceManager.hpp>
#endif

#ifdef _HICR_USE_HWLOC_BACKEND_
  #include <backends/host/hwloc/L1/topologyManager.hpp>
#endif

#ifdef _HICR_USE_ASCEND_BACKEND_
  #include <backends/ascend/L1/topologyManager.hpp>
#endif

namespace HiCR
{

class Runtime;

/**
 * Static singleton of the HiCR runtime class
 */
static Runtime *_runtime;

/**
 * The runtime class represents a singleton that exposes many HiCR's (and its frontends') functionalities with a simplified API
 * and performs backend detect and initialization of a selected set of backends. This class goal is to be FF3 RTS's backend library
 * but it can used by other libraries as well.
 */
class Runtime final
{
  public:

  /**
   * The constructor for the HiCR runtime singleton takes the argc and argv pointers, as passed to the main() function
   *
   * @param[in] pargc Pointer to the argc argument count
   * @param[in] pargv Pointer to the argc argument char arrays
   */
  Runtime(int *pargc, char ***pargv) : _pargc(pargc), _pargv(pargv)
  {
    /////////////////////////// Detecting instance manager

    // Detecting MPI
    #ifdef _HICR_USE_MPI_BACKEND_
        _instanceManager = HiCR::backend::mpi::L1::InstanceManager::createDefault(_pargc, _pargv);
    #endif

    // Detecting YuanRong
    #ifdef _HICR_USE_YUANRONG_BACKEND_
        _instanceManager = HiCR::backend::yuanrong::L1::InstanceManager::createDefault(_pargc, _pargv);
    #endif

    // Checking an instance manager has been found
    if (_instanceManager.get() == nullptr) HICR_THROW_LOGIC("No suitable backend for the instance manager was found.\n");

    /////////////////////////// Detecting communication manager

    // Detecting MPI
    #ifdef _HICR_USE_MPI_BACKEND_
        _communicationManager = std::make_unique<HiCR::backend::mpi::L1::CommunicationManager>();
    #endif

    // Checking an instance manager has been found
    if (_communicationManager.get() == nullptr) HICR_THROW_LOGIC("No suitable backend for the communication manager was found.\n");

    /////////////////////////// Detecting memory manager

    // Detecting MPI
    #ifdef _HICR_USE_MPI_BACKEND_
        _memoryManager = std::make_unique<HiCR::backend::mpi::L1::MemoryManager>();
    #endif

    // Checking an instance manager has been found
    if (_memoryManager.get() == nullptr) HICR_THROW_LOGIC("No suitable backend for the memory manager was found.\n");

    /////////////////////////// Detecting topology managers

    // Clearing current topology managers (if any)
    _topologyManagers.clear();

    // Detecting Hwloc
    #ifdef _HICR_USE_HWLOC_BACKEND_
        _topologyManagers.push_back(std::move(HiCR::backend::host::hwloc::L1::TopologyManager::createDefault()));
    #endif

    // Detecting Ascend
    #ifdef _HICR_USE_ASCEND_BACKEND_
        _topologyManagers.push_back(std::move(HiCR::backend::ascend::L1::TopologyManager::createDefault()));
    #endif

    // Checking at least one topology manager was defined
    if (_topologyManagers.empty() == true) HICR_THROW_LOGIC("No suitable backends for topology managers were found.\n");

    // Creating machine model object
    std::vector<HiCR::L1::TopologyManager *> topologyManagers;
    for (const auto &tm : _topologyManagers) topologyManagers.push_back(tm.get());
    _machineModel = std::make_unique<HiCR::MachineModel>(*_instanceManager, topologyManagers);
  }

  ~Runtime() = default;

  /**
   * This function initializes the HiCR runtime singleton and runs the backen detect and initialization routines
   *
   * @param[in] pargc A pointer to the argc value, as passed to main() necessary for new HiCR instances to count with argument information upon creation.
   * @param[in] pargv A pointer to the argv value, as passed to main() necessary for new HiCR instances to count with argument information upon creation.
   */
  static void initialize(int *pargc, char ***pargv) { _runtime = new Runtime(pargc, pargv); }

  /**
   * This function aborts execution, while trying to bring down all other instances (prevents hang ups). It may be only be called by the coordinator.
   * Calling this function from a worker may result in a hangup.
   *
   * @param[in] errorCode The error code to produce upon abortin execution
   */
  static void abort(const int errorCode) { _runtime->_abort(errorCode); }

  /**
   * Deploys the requested machine model and uses the a user-provided acceptance criteria function to evaluate whether the allotted resources satisfy the request
   *
   * @param[in] requests A vector of instance requests, expressing the requested system's machine model and the tasks that each instance needs to run
   * @param[in] acceptanceCriteriaFc A user-given function that compares the requested topology for a given instance and the one obtained to decide whether it meets the user requirements
   */
  static void deploy(std::vector<HiCR::MachineModel::request_t> &requests, HiCR::MachineModel::topologyAcceptanceCriteriaFc_t acceptanceCriteriaFc) { _runtime->_deploy(requests, acceptanceCriteriaFc, *_runtime->_pargc, *_runtime->_pargv); }

  /**
   * Adds a task that will be a possible target as initial function for a deployed HiCR instance.
   *
   * @param[in] taskName A human-readable string that defines the name of the task. To be executed, this should coincide with the name of a task specified in the machine model requests.
   * @param[in] fc Actual function to be executed upon instantiation
   */
  static void addTask(const std::string &taskName, const HiCR::L1::InstanceManager::RPCFunction_t fc) { return _runtime->_instanceManager->addRPCTarget(taskName, fc); }

  /**
   * Indicates whether the caller HiCR instance is the coordinator instance. There is only one coordiator instance per deployment.
   *
   * @return True, if the caller is the coordinator; false, otherwise.
   */
  static bool isCoordinator() { return _runtime->_instanceManager->getCurrentInstance()->isRootInstance(); }

  /**
   * Indicates whether the caller HiCR instance is a worker instance. There are n-1 workers in an n-sized deployment.
   *
   * @return True, if the caller is a worker; false, otherwise.
   */
  static bool isWorker() { return _runtime->_instanceManager->getCurrentInstance()->isRootInstance() == false; }

  /**
   * Blocking function that sets a worker to start listening for incoming RPCs
   */
  static void listen() { _runtime->_listen(); }

  /**
   * This function returns the unique numerical identifier for the caler instance
   *
   * @return An integer number containing the HiCR instance identifier
   */
  static HiCR::L0::Instance::instanceId_t getInstanceId() { return _runtime->_instanceManager->getCurrentInstance()->getId(); }

  /**
   * This function should be used at the end of execution by all HiCR instances, to correctly finalize the execution environment
   */
  static void finalize()
  {
    // Finalizing instance manager
    _runtime->_finalize();

    // Freeing up memory
    delete _runtime;
  }

  private:

  /**
   * Internal implementation of the finalize function
   */
  void _finalize()
  {
    // If this is the coordinator function, wait for the workers to finalize
    // For this, we wait for them to produce the last return values
    if (isCoordinator() == true)
    {
     // Launching finalization RPC
     for (auto &w : _workers) _instanceManager->launchRPC(*w->getInstance(), "__finalize");

     // Waiting for return ack
     for (auto &w : _workers) _instanceManager->getReturnValue(*w->getInstance());
    }

    // Registering an empty return value to ack on finalization
    if (isWorker() == true) _instanceManager->submitReturnValue(nullptr, 0);

    // Finalizing instance manager
    _instanceManager->finalize();
  }

  /**
   * Internal implementation of the abort function
   */
  void _abort(const int errorCode = 0)
  {
    _instanceManager->abort(errorCode);
  }

  void _initializeWorker()
  {
    printf("[Worker] Initializing Worker Instance...\n"); fflush(stdout);

    ////////// Creating consumer channel to receive variable sized RPCs from the coordinator

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

    // Getting required buffer sizes
    auto tokenSizeBufferSize = HiCR::channel::variableSize::Base::getTokenBufferSize(sizeof(size_t), _HICR_RUNTIME_CHANNEL_COUNT_CAPACITY);

    // Allocating token size buffer as a local memory slot
    auto tokenSizeBufferSlot = _memoryManager->allocateLocalMemorySlot(bufferMemorySpace, tokenSizeBufferSize);

    // Allocating token size buffer as a local memory slot
    auto payloadBufferSlot = _memoryManager->allocateLocalMemorySlot(bufferMemorySpace, _HICR_RUNTIME_CHANNEL_PAYLOAD_CAPACITY);

    // Getting required buffer size for coordination buffers
    auto coordinationBufferSize = HiCR::channel::variableSize::Base::getCoordinationBufferSize();

    // Allocating coordination buffers
    auto coordinationBufferMessageSizes    = _memoryManager->allocateLocalMemorySlot(bufferMemorySpace, coordinationBufferSize);
    auto coordinationBufferMessagePayloads = _memoryManager->allocateLocalMemorySlot(bufferMemorySpace, coordinationBufferSize);

    // Initializing coordination buffers
    HiCR::channel::variableSize::Base::initializeCoordinationBuffer(coordinationBufferMessageSizes);
    HiCR::channel::variableSize::Base::initializeCoordinationBuffer(coordinationBufferMessagePayloads);

    // Getting instance id for memory slot exchange
    const auto instanceId = _instanceManager->getCurrentInstance()->getId(); 
     
    // Exchanging local memory slots to become global for them to be used by the remote end
    _communicationManager->exchangeGlobalMemorySlots(_HICR_RUNTIME_CHANNEL_WORKER_SIZES_BUFFER_TAG,                      {{instanceId, tokenSizeBufferSlot}});
    _communicationManager->fence(_HICR_RUNTIME_CHANNEL_WORKER_SIZES_BUFFER_TAG);

    _communicationManager->exchangeGlobalMemorySlots(_HICR_RUNTIME_CHANNEL_WORKER_PAYLOAD_BUFFER_TAG,                    {{instanceId, payloadBufferSlot}});
    _communicationManager->fence(_HICR_RUNTIME_CHANNEL_WORKER_PAYLOAD_BUFFER_TAG);

    _communicationManager->exchangeGlobalMemorySlots(_HICR_RUNTIME_CHANNEL_WORKER_COORDINATION_BUFFER_SIZES_TAG,         {{instanceId, coordinationBufferMessageSizes}});
    _communicationManager->fence(_HICR_RUNTIME_CHANNEL_WORKER_COORDINATION_BUFFER_SIZES_TAG);

    _communicationManager->exchangeGlobalMemorySlots(_HICR_RUNTIME_CHANNEL_WORKER_COORDINATION_BUFFER_PAYLOADS_TAG,      {{instanceId, coordinationBufferMessagePayloads}});
    _communicationManager->fence(_HICR_RUNTIME_CHANNEL_WORKER_COORDINATION_BUFFER_PAYLOADS_TAG);

    _communicationManager->exchangeGlobalMemorySlots(_HICR_RUNTIME_CHANNEL_COORDINATOR_COORDINATION_BUFFER_SIZES_TAG,    {});
    _communicationManager->fence(_HICR_RUNTIME_CHANNEL_COORDINATOR_COORDINATION_BUFFER_SIZES_TAG);
    
    _communicationManager->exchangeGlobalMemorySlots(_HICR_RUNTIME_CHANNEL_COORDINATOR_COORDINATION_BUFFER_PAYLOADS_TAG, {});
    _communicationManager->fence(_HICR_RUNTIME_CHANNEL_COORDINATOR_COORDINATION_BUFFER_PAYLOADS_TAG); 

    // Obtaining the globally exchanged memory slots
    auto workerMessageSizesBuffer            = _communicationManager->getGlobalMemorySlot(_HICR_RUNTIME_CHANNEL_WORKER_SIZES_BUFFER_TAG,                      instanceId);
    auto workerMessagePayloadBuffer          = _communicationManager->getGlobalMemorySlot(_HICR_RUNTIME_CHANNEL_WORKER_PAYLOAD_BUFFER_TAG,                    instanceId);
    auto coordinatorSizesCoordinatorBuffer   = _communicationManager->getGlobalMemorySlot(_HICR_RUNTIME_CHANNEL_COORDINATOR_COORDINATION_BUFFER_SIZES_TAG,    instanceId);
    auto coordinatorPayloadCoordinatorBuffer = _communicationManager->getGlobalMemorySlot(_HICR_RUNTIME_CHANNEL_COORDINATOR_COORDINATION_BUFFER_PAYLOADS_TAG, instanceId);

    // Creating channel
    auto consumerChannel = HiCR::channel::variableSize::SPSC::Consumer(
      *_communicationManager.get(),
      workerMessagePayloadBuffer,
      workerMessageSizesBuffer,
      coordinationBufferMessageSizes,
      coordinationBufferMessagePayloads,
      coordinatorSizesCoordinatorBuffer,
      coordinatorPayloadCoordinatorBuffer,
      _HICR_RUNTIME_CHANNEL_PAYLOAD_CAPACITY,
      sizeof(uint8_t),
      _HICR_RUNTIME_CHANNEL_COUNT_CAPACITY
    );

    // Waiting for initial message from coordinator
    while (consumerChannel.getDepth() == 0) consumerChannel.updateDepth();

    // Get internal pointer of the token buffer slot and the offset
    auto payloadBufferPtr = (const char*) payloadBufferSlot->getPointer();
    auto offset = consumerChannel.peek()[0];
    consumerChannel.pop();

    // Printing message
    printf("[Worker] Message from the coordinator: '%s'\n", &payloadBufferPtr[offset]);

  }

  void initializeCoordinator()
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
      const auto workerInstanceId = _workers[i]->getInstance()->getId();

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
      const auto workerInstanceId = worker->getInstance()->getId();

      // Obtaining the globally exchanged memory slots
      auto workerMessageSizesBuffer            = _communicationManager->getGlobalMemorySlot(_HICR_RUNTIME_CHANNEL_WORKER_SIZES_BUFFER_TAG,                      workerInstanceId);
      auto workerMessagePayloadBuffer          = _communicationManager->getGlobalMemorySlot(_HICR_RUNTIME_CHANNEL_WORKER_PAYLOAD_BUFFER_TAG,                    workerInstanceId);
      auto coordinatorSizesCoordinatorBuffer   = _communicationManager->getGlobalMemorySlot(_HICR_RUNTIME_CHANNEL_COORDINATOR_COORDINATION_BUFFER_SIZES_TAG,    workerInstanceId);
      auto coordinatorPayloadCoordinatorBuffer = _communicationManager->getGlobalMemorySlot(_HICR_RUNTIME_CHANNEL_COORDINATOR_COORDINATION_BUFFER_PAYLOADS_TAG, workerInstanceId);

      // Creating channel
      auto producerChannel = HiCR::channel::variableSize::SPSC::Producer(
        *_communicationManager.get(),
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
   * Internal implementation of the listen function
   */
  void _listen()
  {
    // Flag to indicate whether the worker should continue listening
    bool continueListening = true;

    // Adding RPC targets, specifying a name and the execution unit to execute
    _instanceManager->addRPCTarget("__finalize", [&]() { continueListening = false; });
    _instanceManager->addRPCTarget("__initializeWorker", [this]() { this->_initializeWorker(); });

    // Listening for RPC requests
    while (continueListening == true) _instanceManager->listen();
  }

  /**
   * Internal implementation of the deploy function
   */
  void _deploy(std::vector<HiCR::MachineModel::request_t> &requests, HiCR::MachineModel::topologyAcceptanceCriteriaFc_t acceptanceCriteriaFc, int argc, char *argv[])
  {
    // Execute requests by finding or creating an instance that matches their topology requirements
    try
    {
      _machineModel->deploy(requests, acceptanceCriteriaFc);
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
         auto worker = std::make_unique<runtime::Worker>(r.taskName, in);

         // Adding worker to the set
         _workers.push_back(std::move(worker));
      } 

    // Launching channel creation routine for every worker
    for (auto &w : _workers) _instanceManager->launchRPC(*w->getInstance(), "__initializeWorker");

    // Initializing coordinator instance
    initializeCoordinator();

    // Launching worker's entry point function     
    for (auto &w : _workers) _instanceManager->launchRPC(*w->getInstance(), w->getEntryPoint()); 
  }

  /**
   * Storage pointer for argc
   */
  int *const _pargc;

  /**
   * Storage pointer for argv
   */
  char ***const _pargv;

  /**
   * Detected instance manager to use for detecting and creating HiCR instances (only one allowed)
   */
  std::unique_ptr<HiCR::L1::InstanceManager> _instanceManager = nullptr;

  /**
   * Detected communication manager to use for communicating between HiCR instances
   */
  std::unique_ptr<HiCR::L1::CommunicationManager> _communicationManager = nullptr;

  /**
   * Detected memory manager to use for allocating memory
   */
  std::unique_ptr<HiCR::L1::MemoryManager> _memoryManager = nullptr;

  /**
   * Detected topology managers to use for resource discovery
   */
  std::vector<std::unique_ptr<HiCR::L1::TopologyManager>> _topologyManagers;

   /**
   * Machine model object for deployment
   */
  std::unique_ptr<HiCR::MachineModel> _machineModel;


  /**
   * Storage for the deployed workers. This object is only mantained and usable by the coordinator
   */
  std::vector<std::unique_ptr<HiCR::runtime::Worker>> _workers;
};

} // namespace HiCR