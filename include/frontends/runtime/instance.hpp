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
#include <hicr/L1/instanceManager.hpp>
#include <hicr/L1/memoryManager.hpp>
#include <hicr/L1/communicationManager.hpp>
#include <hicr/L1/topologyManager.hpp>
#include <frontends/machineModel/machineModel.hpp>

// For interoperability with YuanRong, we bifurcate implementations using different includes
#ifdef _HICR_USE_YUANRONG_BACKEND_
  #include <frontends/runtime/dataObject/yuanrong/dataObject.hpp>
  #include <frontends/runtime/channel/yuanrong/producerChannel.hpp>
  #include <frontends/runtime/channel/yuanrong/consumerChannel.hpp>
#else
  #include "dataObject/mpi/dataObject.hpp"
  #include "channel/hicr/producerChannel.hpp"
  #include "channel/hicr/consumerChannel.hpp"
#endif

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
  Instance(
    HiCR::L1::InstanceManager &instanceManager,
    HiCR::L1::CommunicationManager &communicationManager,
    HiCR::L1::MemoryManager &memoryManager,
    std::vector<HiCR::L1::TopologyManager *> &topologyManagers,
    HiCR::MachineModel &machineModel) : _HiCRInstance(instanceManager.getCurrentInstance()),
                                        _instanceManager(&instanceManager),
                                        _communicationManager(&communicationManager),
                                        _memoryManager(&memoryManager),
                                        _topologyManagers(topologyManagers),
                                        _machineModel(&machineModel)
  {
    // Getting instance ids into a sorted vector
    for (const auto& instance : _instanceManager->getInstances()) _instanceIds.push_back(instance->getId());
    std::sort(_instanceIds.begin(), _instanceIds.end());
  }
  
  virtual ~Instance() = default;

  /**
   * Initializer function for the instance
   */
  virtual void initialize() = 0;

  /**
   * This function should be used at the end of execution by all HiCR instances, to correctly finalize the execution environment
   */
  virtual void finalize() = 0;

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
  __USED__ inline std::shared_ptr<DataObject> createDataObject(void *buffer, const size_t size)
  {
    DataObject::dataObjectId_t dataObjectId;

    // Generate a new UUID
    auto uuid = boost::uuids::random_generator()();

    // Truncate it to fit into the data object id
    std::memcpy(&dataObjectId, &uuid.data, sizeof(DataObject::dataObjectId_t));

    return std::make_shared<DataObject>(buffer, size, dataObjectId, _instanceManager->getCurrentInstance()->getId());
  }

  protected:

  /**
   * Internal implementation of the abort function
   *
   * @param[in] errorCode The error code to produce upon abort
   */
  void _abort(const int errorCode = 0)
  {
    _instanceManager->abort(errorCode);
  }

  /**
   *  The ids of other instances, sorted by Id
  */
  std::vector<HiCR::L0::Instance::instanceId_t> _instanceIds;

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

  inline void initializeChannels();

  /**
   * Producer channels for sending messages to all other instances
  */
  std::vector<std::shared_ptr<runtime::ProducerChannel>> _producerChannels;

  /**
   * Consumer channels for sending messages to all other instances
  */
  std::vector<std::shared_ptr<runtime::ConsumerChannel>> _consumerChannels;
};

// For interoperability with YuanRong, we bifurcate implementations using different includes
#ifdef _HICR_USE_YUANRONG_BACKEND_
  #include <frontends/runtime/channel/yuanrong/initializeChannelsImpl.hpp>
#else
  #include "channel/hicr/initializeChannelsImpl.hpp"
#endif

} // namespace runtime

} // namespace HiCR
