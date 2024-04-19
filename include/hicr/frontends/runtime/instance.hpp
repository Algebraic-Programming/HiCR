/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file worker.hpp
 * @brief Provides a definition for the Runtime worker class.
 * @author S. M. Martin
 * @date 5/2/2024
 */

#pragma once

#include <vector>
#include <algorithm>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <hicr/core/L1/instanceManager.hpp>
#include <hicr/core/L1/memoryManager.hpp>
#include <hicr/core/L1/communicationManager.hpp>
#include <hicr/core/L1/topologyManager.hpp>
#include <hicr/frontends/machineModel/machineModel.hpp>

// For interoperability with YuanRong, we bifurcate implementations using different includes
#ifdef _HICR_USE_YUANRONG_BACKEND_
  #include <hicr/frontends/runtime/dataObject/yuanrong/dataObject.hpp>
  #include <hicr/frontends/runtime/channel/yuanrong/producerChannel.hpp>
  #include <hicr/frontends/runtime/channel/yuanrong/consumerChannel.hpp>
#else
  #ifdef _HICR_USE_MPI_BACKEND_
    #include "dataObject/mpi/dataObject.hpp"
    #include "channel/hicr/producerChannel.hpp"
    #include "channel/hicr/consumerChannel.hpp"
  #else
    #include "dataObject/local/dataObject.hpp"
    #include "channel/hicr/producerChannel.hpp"
    #include "channel/hicr/consumerChannel.hpp"
  #endif // _HICR_USE_MPI_BACKEND_
#endif   // _HICR_USE_YUANRONG_BACKEND_

namespace HiCR
{

namespace runtime
{

/**
 * Defines the worker class, which represents a self-contained instance of HiCR with access to compute and memory resources.
 *
 * Workers may be created during runtime (if the process managing backend allows for it), or activated/suspended on demand.
 */
class Instance
{
  public:

  /**
   * Storage for inter-instance message information
  */
  struct message_t
  {
    /// Pointer to the message's data allocation
    const void *data = nullptr;

    /// Size of the message
    size_t size = 0;
  };

  Instance() = delete;

  /**
   * Constructor for the coordinator class
   *
   * @param[in] instanceManager The instance manager backend to use
   * @param[in] communicationManager The communication manager backend to use
   * @param[in] memoryManager The memory manager backend to use
   * @param[in] topologyManagers The topology managers backend to use to discover the system's resources
   * @param[in] machineModel The machine model to use to deploy the workers
   */
  Instance(HiCR::L1::InstanceManager                *instanceManager,
           HiCR::L1::CommunicationManager           *communicationManager,
           HiCR::L1::MemoryManager                  *memoryManager,
           std::vector<HiCR::L1::TopologyManager *> &topologyManagers,
           HiCR::MachineModel                       *machineModel)
    : _HiCRInstance(instanceManager->getCurrentInstance()),
      _instanceManager(instanceManager),
      _communicationManager(communicationManager),
      _memoryManager(memoryManager),
      _topologyManagers(topologyManagers),
      _machineModel(machineModel)
  {}

  virtual ~Instance() = default;

  /**
   * Returns the internal HiCR instance object for the caller instance
   *
   * @return The pointer to the internal HiCR instance
   */
  HiCR::L0::Instance *getHiCRInstance() const { return _HiCRInstance.get(); }

  /**
   * Function to obtain the instance manager from the instance
   *
   * @return The HiCR runtime's instance manager
   */
  HiCR::L1::InstanceManager *getInstanceManager() const { return _instanceManager; }

  /**
   * Function to obtain the communication manager from the instance
   *
   * @return The HiCR runtime's communication manager
   */
  HiCR::L1::InstanceManager *getCommunicationManager() const { return _instanceManager; }

  /**
   * Function to obtain the memory manager from the instance
   *
   * @return The HiCR runtime's memory manager
   */
  HiCR::L1::MemoryManager *getMemoryManager() const { return _memoryManager; }

  /**
   * Function to obtain the topology managers from the instance
   *
   * @return The HiCR runtime's topology managers
   */
  std::vector<HiCR::L1::TopologyManager *> getTopologyManagers() const { return _topologyManagers; }

  /**
   * Function to get machine model object from the instance
   *
   * @return The HiCR machine model
   */
  HiCR::MachineModel *getMachineModel() const { return _machineModel; }

  /**
   * Requests the creation of a new data object.
   *
   * This adds the internal counter for the assignment of unique identifiers to new data objects
   *
   * @param[in] buffer A pointer to the internal buffer to assign to the data object
   * @param[in] size The size of the internal buffer to use
   * @return A shared pointer to the newly created data object
   */
  __INLINE__ std::shared_ptr<DataObject> createDataObject(void *buffer, const size_t size)
  {
    DataObject::dataObjectId_t dataObjectId;

    // Generate a new UUID
    auto uuid = boost::uuids::random_generator()();

    // Truncate it to fit into the data object id
    std::memcpy(&dataObjectId, &uuid.data, sizeof(DataObject::dataObjectId_t));

    return std::make_shared<DataObject>(buffer, size, dataObjectId, _instanceManager->getCurrentInstance()->getId(), _instanceManager->getSeed());
  }

  /**
   * Allows a worker to obtain a data object by id from the coordinator instance
   *
   * This is a blocking function.
   * The data object must be published (either before or after this call) by the coordinator for this function to succeed.
   *
   * @param[in] dataObjectId The id of the data object to get from the coordinator instance
   * @return A shared pointer to the obtained data object
   */
  __INLINE__ std::shared_ptr<DataObject> getDataObject(const DataObject::dataObjectId_t dataObjectId)
  {
    // Getting instance id of coordinator instance
    const auto coordinatorId     = _instanceManager->getRootInstanceId();
    const auto currentInstanceId = _instanceManager->getCurrentInstance()->getId();
    // Creating data object from id and remote instance id
    return DataObject::getDataObject(dataObjectId, coordinatorId, currentInstanceId, _instanceManager->getSeed());
  }

  /**
   * Asynchronously sends a binary message (buffer + size) to another instance
   *
   * @param[in] instanceId The id of the instance to which to send the message
   * @param[in] messagePtr The pointer to the message buffer
   * @param[in] messageSize The message size in bytes
   */
  __INLINE__ void sendMessage(const HiCR::L0::Instance::instanceId_t instanceId, void *messagePtr, size_t messageSize);

  /**
   * Synchronous function to receive a message from another instance
   *
   * @param[in] instanceId The id of the instance for which channel we check for incoming messages
   * @param[in] isAsync Whether the function must return immediately if no message was found
   * @return A pair containing a pointer to the start of the message binary data and the message's size
   */
  __INLINE__ message_t recvMessage(const HiCR::L0::Instance::instanceId_t instanceId, const bool isAsync = false);

  /**
   * Asynchronous function to receive a message from another instance
   *
   * This function returns immediately.
   *
   * @param[in] instanceId The id of the instance for which channel we check for incoming messages
   * @return A pair containing a pointer to the start of the message binary data and the message's size. The pointer will be NULL if no messages were there when called.
   */
  __INLINE__ message_t recvMessageAsync(const HiCR::L0::Instance::instanceId_t instanceId) { return recvMessage(instanceId, true); }

  /**
   * Function to initialize producer and consumer channels with all the rest of the instances
   */
  __INLINE__ void initializeChannels();

  /**
  * Prompts the currently running instance to start listening for incoming RPCs
  */
  __INLINE__ void listen()
  {
    // Flag to indicate whether the worker should continue listening
    bool continueListening = true;

    // Adding RPC targets, specifying a name and the execution unit to execute
    _instanceManager->addRPCTarget("__finalize", [&]() { continueListening = false; });
    _instanceManager->addRPCTarget("__initializeChannels", [this]() { initializeChannels(); });

    // Listening for RPC requests
    while (continueListening == true) _instanceManager->listen();

    // Finalizing by sending a last acknowledgment as return value to the "__finalize" RPC
    uint8_t ack = 0;

    // Registering an empty return value to ack on finalization
    _instanceManager->submitReturnValue(&ack, sizeof(ack));

    // Finalizing instance manager
    _instanceManager->finalize();

    // Exiting now
    exit(0);
  }

  protected:

  /**
   * Internal HiCR instance represented by this runtime instance
   */
  const std::shared_ptr<HiCR::L0::Instance> _HiCRInstance;

  /**
   * Detected instance manager to use for detecting and creating HiCR instances (only one allowed)
   */
  HiCR::L1::InstanceManager *const _instanceManager = nullptr;

  /**
   * Detected communication manager to use for communicating between HiCR instances
   */
  HiCR::L1::CommunicationManager *const _communicationManager = nullptr;

  /**
   * Detected memory manager to use for allocating memory
   */
  HiCR::L1::MemoryManager *const _memoryManager;

  /**
   * Detected topology managers to use for resource discovery
   */
  std::vector<HiCR::L1::TopologyManager *> _topologyManagers;

  /**
   * Machine model object for deployment
   */
  HiCR::MachineModel *const _machineModel;

  /**
   * Global counter for the current data object pertaining to this instance
   */
  DataObject::dataObjectId_t _currentDataObjectId = 0;

  /**
   * Producer channels for sending messages to all other instances
   */
  std::map<HiCR::L0::Instance::instanceId_t, std::shared_ptr<runtime::ProducerChannel>> _producerChannels;

  /**
   * Consumer channels for sending messages to all other instances
   */
  std::map<HiCR::L0::Instance::instanceId_t, std::shared_ptr<runtime::ConsumerChannel>> _consumerChannels;
};

// For interoperability with YuanRong, we bifurcate implementations using different includes
#ifdef _HICR_USE_YUANRONG_BACKEND_
  #include <hicr/frontends/runtime/channel/yuanrong/channelsImpl.hpp>
#else
  #include "channel/hicr/channelsImpl.hpp"
#endif

} // namespace runtime

} // namespace HiCR
