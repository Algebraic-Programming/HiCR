#pragma once

#include <unordered_set>
#include "common.hpp"
#include "nlohmann_json/json.hpp"
#include <hicr/L0/topology.hpp>
#include <hicr/L1/instanceManager.hpp>
#include <backends/host/hwloc/L1/memoryManager.hpp>
#include <backends/host/hwloc/L1/topologyManager.hpp>
#include <backends/host/L0/device.hpp>
#include <backends/host/L0/computeResource.hpp>
#include <backends/host/L0/memorySpace.hpp>

#ifdef _HICR_USE_ASCEND_BACKEND_
#include <backends/ascend/L0/device.hpp>
#endif

// This struct hold the information of an instance to create, as described by the machine model
struct requestedInstance_t
{
  // Task name to execute
  std::string taskName;

  // Number of replicas of this instance to create
  size_t replicaCount;

  // Requested topology for this instance, in HiCR format
  HiCR::L0::Topology topology;
};

void finalizeExecution(HiCR::L1::InstanceManager &instanceManager, const int returnCode = 0)
{
  // Querying instance list
  auto &instances = instanceManager.getInstances();

  // Getting the pointer to our own (coordinator) instance
  auto coordinator = instanceManager.getCurrentInstance();

  // Requesting workers to abort and printing error message 
  for (const auto &instance : instances)
    if (instance->getId() != coordinator->getId())
      instanceManager.execute(*instance, PROCESSING_UNIT_ID, FINALIZATION_EXECUTION_UNIT_ID);

  HiCR::finalize();
  exit(returnCode);
}

HiCR::L0::Topology parseTopology(const nlohmann::json& topologyJson)
{
  // Storage for the HiCR-formatted topology to create
  HiCR::L0::Topology topology;

  // Parsing CPU cores entry
  if (topologyJson.contains("CPU Cores") == false) throw std::runtime_error("Error: the requested instance topoogy does not contain a 'CPU Cores' entry\n");
  if (topologyJson["CPU Cores"].is_number_unsigned() == false) throw std::runtime_error("Error: The instance topology 'CPU Cores' entry is not a positive number\n");
  size_t numCores = topologyJson["CPU Cores"].get<size_t>();

  // Creating list of compute resources (CPU cores / processing units)
  HiCR::L0::Device::computeResourceList_t computeResources;
  for (size_t i = 0; i < numCores; i++) computeResources.insert(std::make_shared<HiCR::backend::host::L0::ComputeResource>());

  // Parsing host RAM size entry
  if (topologyJson.contains("Host RAM Size (Gb)") == false) throw std::runtime_error("Error: the requested instance topoogy does not contain a 'Host RAM Size (Gb)' entry\n");
  if (topologyJson["Host RAM Size (Gb)"].is_number() == false) throw std::runtime_error("Error: The instance topology 'Host RAM Size (Gb)' entry is not a number\n");
  auto hostRamSize = (size_t)std::ceil(topologyJson["Host RAM Size (Gb)"].get<double>() * 1024.0 * 1024.0 * 1024.0);

  // Creating list of memory spaces (only one, with the total host memory)
  HiCR::L0::Device::memorySpaceList_t memorySpaces ( { std::make_shared<HiCR::backend::host::L0::MemorySpace>( hostRamSize ) });

  // Creating the CPU (NUMA Domain) device type with the assigned number of cores
  auto hostDevice = std::make_shared<HiCR::backend::host::L0::Device>(0, computeResources, memorySpaces);

  // Adding host device to the topology
  topology.addDevice(hostDevice);

  #ifdef _HICR_USE_ASCEND_BACKEND_

  // Parsing ascend device list
  if (topologyJson.contains("Ascend Devices") == false) throw std::runtime_error("Error: the requested instance topoogy does not contain a 'Ascend Devices' entry\n");
  if (topologyJson["Ascend Devices"].is_number_unsigned() == false) throw std::runtime_error("Error: The instance topology 'Ascend Devices' entry is not a positive number\n");
  size_t ascendDeviceCount = topologyJson["Ascend Devices"].get<size_t>();

  // Adding requested ascend devices, if any
  for (size_t i = 0; i < ascendDeviceCount; i++) topology.addDevice(std::make_shared<HiCR::backend::ascend::L0::Device>());
  
  #endif

  // Returning the HiCR-formatted topology
  return topology;
}

std::vector<std::shared_ptr<requestedInstance_t>> parseMachineModel(const nlohmann::json& machineModelJson)
{
  // Storage for the parsed instances from the machine model
  std::vector<std::shared_ptr<requestedInstance_t>> requestedInstances;  

  // Checking for correct format in the machine model
  if (machineModelJson.contains("Instances") == false) throw std::runtime_error("Error: the machine model does not contain an 'Instances' entry\n");
  if (machineModelJson["Instances"].is_array() == false) throw std::runtime_error("Error: The 'Instances' entry in the machine model is not an array\n");

  // Now iterating over all requested instances
  for (const auto& instance : machineModelJson["Instances"])
  {
    // Storage for the new requested instance to add
    auto newRequestedInstance = std::make_shared<requestedInstance_t>();

    // Parsing task name
    if (instance.contains("Task") == false) throw std::runtime_error("Error: the requested instance does not contain a 'Task' entry\n"); 
    if (instance["Task"].is_string() == false) throw std::runtime_error("Error: The instance 'Task' entry is not a string\n");
    newRequestedInstance->taskName = instance["Task"].get<std::string>();

    // Parsing replica count
    if (instance.contains("Replicas") == false) throw std::runtime_error("Error: the requested instance does not contain a 'Replicas' entry\n");
    if (instance["Replicas"].is_number_unsigned() == false) throw std::runtime_error("Error: The instance 'Replicas' entry is not an unsigned number\n");
    newRequestedInstance->replicaCount = instance["Replicas"].get<size_t>();

    // Parsing requested topology
    if (instance.contains("Topology") == false) throw std::runtime_error("Error: the requested instance does not contain a 'Topology' entry\n");
    if (instance["Topology"].is_object() == false) throw std::runtime_error("Error: The instance 'Topology' entry is not an object\n");
    newRequestedInstance->topology = parseTopology(instance["Topology"]);

    // Adding newly requested instance to the collection
    requestedInstances.push_back(newRequestedInstance);
  }

  // Returning parsed requested instances
  return requestedInstances;
}


void coordinatorFc(HiCR::L1::InstanceManager &instanceManager, const std::string& machineModelFilePath)
{
  // Getting the pointer to our own (coordinator) instance
  auto coordinator = instanceManager.getCurrentInstance();

  // Reading from machine model file
  std::string machineModelRaw;
  auto status = loadStringFromFile(machineModelRaw, machineModelFilePath);
  if (status == false) {  fprintf(stderr, "Error: could not read from machine model file: '%s'\n", machineModelFilePath.c_str()); finalizeExecution(instanceManager, -1); }

  // Parsing received machine model file
  nlohmann::json machineModelJson;
  try { machineModelJson = nlohmann::json::parse(machineModelRaw); }
   catch (const std::exception& e)
    {  fprintf(stderr, "Error: could not parse JSON from machine model file: '%s'. Reason: '%s'\n", machineModelFilePath.c_str(), e.what()); finalizeExecution(instanceManager, -1); }

  // Parsing the machine model into its requested instances. Here the vector implies ordering, which allows the user specify which instances need to be allocated first
  std::vector<std::shared_ptr<requestedInstance_t>> requestedInstances;
  try { requestedInstances = parseMachineModel(machineModelJson); }
   catch (const std::exception& e)
    {  fprintf(stderr, "Error while parsing the machine model. Reason: '%s'\n", e.what());  finalizeExecution(instanceManager, -1); }

  // Obtaining the currently launched instances
  auto &instances = instanceManager.getInstances();

  // Obtaining each of the instances HiCR topology
  for (const auto &instance : instances)
  if (instance->getId() != coordinator->getId())
  {
    // Running the RPC that obtains the instance's serialized topology
    instanceManager.execute(*instance, PROCESSING_UNIT_ID, TOPOLOGY_EXECUTION_UNIT_ID);

    // Gathering the return value
    auto returnValue = instanceManager.getReturnValue(*instance);

    // Receiving raw serialized topology information from the worker
    std::string serializedTopology = (char *)returnValue->getPointer();

    // Parsing serialized raw topology into a json object
    auto topologyJson = nlohmann::json::parse(serializedTopology);

    // HiCR topology object to obtain
    HiCR::L0::Topology topology;

    // Obtaining the topology from the serialized object
    topology.merge(HiCR::backend::host::hwloc::L1::TopologyManager::deserializeTopology(topologyJson));

    #ifdef _HICR_USE_ASCEND_BACKEND_
    topology.merge(HiCR::backend::ascend::L1::TopologyManager::deserializeTopology(topologyJson));
    #endif // _HICR_USE_ASCEND_BACKEND_
  }

  // Printing instance information and invoking a simple RPC if its not ourselves
  for (const auto &instance : instances)
    if (instance->getId() != coordinator->getId())
      instanceManager.execute(*instance, PROCESSING_UNIT_ID, TASK_A_EXECUTION_UNIT_ID);

  // Getting return values from the RPCs containing each of the worker's topology
  for (const auto &instance : instances) if (instance != coordinator)
  {
    // Getting return value as a memory slot
    auto returnValue = instanceManager.getReturnValue(*instance);

    // Printing return value
    printf("[Coordinator] Received from instance %lu: '%s'\n", instance->getId(), (const char*)returnValue->getPointer());
  }

  // Finalizing execution for all instances
  finalizeExecution(instanceManager);
}
