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
#include <frontends/machineModel/machineModel.hpp>

#ifdef _HICR_USE_ASCEND_BACKEND_
#include <backends/ascend/L0/device.hpp>
#endif

// This struct hold the information of a detected instance including its topology
struct detectedInstance_t
{
  // Pointer to the detected instance
  std::shared_ptr<HiCR::L0::Instance> instance;

  // Detected topology of the given instance
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
      instanceManager.execute(*instance, "Finalize");

  HiCR::finalize();
  exit(returnCode);
}

HiCR::L0::Topology parseTopology(const nlohmann::json& topologyJson)
{
  // Storage for the HiCR-formatted topology to create
  HiCR::L0::Topology topology;

  // Parsing CPU cores entry
  if (topologyJson.contains("CPU Cores") == false) throw std::runtime_error("the requested instance topoogy does not contain a 'CPU Cores' entry\n");
  if (topologyJson["CPU Cores"].is_number_unsigned() == false) throw std::runtime_error("The instance topology 'CPU Cores' entry is not a positive number\n");
  size_t numCores = topologyJson["CPU Cores"].get<size_t>();

  // Creating list of compute resources (CPU cores / processing units)
  HiCR::L0::Device::computeResourceList_t computeResources;
  for (size_t i = 0; i < numCores; i++) computeResources.insert(std::make_shared<HiCR::backend::host::L0::ComputeResource>());

  // Parsing host RAM size entry
  if (topologyJson.contains("Host RAM Size (Gb)") == false) throw std::runtime_error("the requested instance topoogy does not contain a 'Host RAM Size (Gb)' entry\n");
  if (topologyJson["Host RAM Size (Gb)"].is_number() == false) throw std::runtime_error("The instance topology 'Host RAM Size (Gb)' entry is not a number\n");
  auto hostRamSize = (size_t)std::ceil(topologyJson["Host RAM Size (Gb)"].get<double>() * 1024.0 * 1024.0 * 1024.0);

  // Creating list of memory spaces (only one, with the total host memory)
  HiCR::L0::Device::memorySpaceList_t memorySpaces ( { std::make_shared<HiCR::backend::host::L0::MemorySpace>( hostRamSize ) });

  // Creating the CPU (NUMA Domain) device type with the assigned number of cores
  auto hostDevice = std::make_shared<HiCR::backend::host::L0::Device>(0, computeResources, memorySpaces);

  // Adding host device to the topology
  topology.addDevice(hostDevice);

  #ifdef _HICR_USE_ASCEND_BACKEND_

  // Parsing ascend device list
  if (topologyJson.contains("Ascend Devices") == false) throw std::runtime_error("the requested instance topoogy does not contain a 'Ascend Devices' entry\n");
  if (topologyJson["Ascend Devices"].is_number_unsigned() == false) throw std::runtime_error("The instance topology 'Ascend Devices' entry is not a positive number\n");
  size_t ascendDeviceCount = topologyJson["Ascend Devices"].get<size_t>();

  // Adding requested ascend devices, if any
  for (size_t i = 0; i < ascendDeviceCount; i++) topology.addDevice(std::make_shared<HiCR::backend::ascend::L0::Device>());
  
  #endif

  // Returning the HiCR-formatted topology
  return topology;
}

bool isTopologySubset(const HiCR::L0::Topology& a, const HiCR::L0::Topology& b)
{
  // For this example, it suffices that topology B has more or equal:
  //  + Total Core Count (among all NUMA domains)
  //  + Total RAM size (among all NUMA domains)
  //  + Ascend devices
  // than topology A.

  size_t tACoreCount = 0;
  size_t tBCoreCount = 0;

  size_t tAMemSize = 0;
  size_t tBMemSize = 0;

  size_t tAAscendDeviceCount = 0;
  size_t tBAscendDeviceCount = 0;

  // Processing topology A
  for (const auto& d: a.getDevices()) 
  {
    const auto deviceType = d->getType();

    // If it's a NUMA Domain device, then its about host requirements
    if (deviceType == "NUMA Domain")
    {
      // Casting to a host device
      auto hostDev = std::dynamic_pointer_cast<HiCR::backend::host::L0::Device>(d);

      // Adding corresponding counts
      tACoreCount += hostDev->getComputeResourceList().size();
      tAMemSize += (*hostDev->getMemorySpaceList().begin())->getSize();
    }

    // It it's an Ascend device, increment the ascend device counter
    if (deviceType == "Ascend Device") tAAscendDeviceCount++;
  } 

  // Processing topology B
  for (const auto& d: b.getDevices()) 
  {
    const auto deviceType = d->getType();

    // If it's a NUMA Domain device, then its about host requirements
    if (deviceType == "NUMA Domain")
    {
      // Casting to a host device
      auto hostDev = std::dynamic_pointer_cast<HiCR::backend::host::L0::Device>(d);

      // Adding corresponding counts
      tBCoreCount += hostDev->getComputeResourceList().size();
      tBMemSize += (*hostDev->getMemorySpaceList().begin())->getSize();
    }

    // It it's an Ascend device, increment the ascend device counter
    if (deviceType == "Ascend Device") tBAscendDeviceCount++;
  } 

  // Evaluating criteria
  if (tACoreCount > tBCoreCount) return false;
  if (tAMemSize   > tBMemSize) return false;
  if (tAAscendDeviceCount > tBAscendDeviceCount) return false;

  // If no criteria failed, return true
  return true;
}

std::vector<HiCR::MachineModel::request_t> parseMachineModel(const nlohmann::json& machineModelJson)
{
  // Storage for the parsed instances from the machine model
  std::vector<HiCR::MachineModel::request_t> requests;  

  // Checking for correct format in the machine model
  if (machineModelJson.contains("Instances") == false) throw std::runtime_error("the machine model does not contain an 'Instances' entry\n");
  if (machineModelJson["Instances"].is_array() == false) throw std::runtime_error("The 'Instances' entry in the machine model is not an array\n");

  // Now iterating over all requested instances
  for (const auto& instance : machineModelJson["Instances"])
  {
    // Storage for the new requested instance to add
    HiCR::MachineModel::request_t newRequestedInstance;

    // Parsing task name
    if (instance.contains("Task") == false) throw std::runtime_error("the requested instance does not contain a 'Task' entry\n"); 
    if (instance["Task"].is_string() == false) throw std::runtime_error("The instance 'Task' entry is not a string\n");
    newRequestedInstance.taskName = instance["Task"].get<std::string>();

    // Parsing replica count
    if (instance.contains("Replicas") == false) throw std::runtime_error("the requested instance does not contain a 'Replicas' entry\n");
    if (instance["Replicas"].is_number_unsigned() == false) throw std::runtime_error("The instance 'Replicas' entry is not an unsigned number\n");
    newRequestedInstance.replicaCount = instance["Replicas"].get<size_t>();

    // Parsing requested topology
    if (instance.contains("Topology") == false) throw std::runtime_error("the requested instance does not contain a 'Topology' entry\n");
    if (instance["Topology"].is_object() == false) throw std::runtime_error("The instance 'Topology' entry is not an object\n");
    newRequestedInstance.topology = parseTopology(instance["Topology"]);

    // Adding newly requested instance to the collection
    requests.push_back(newRequestedInstance);
  }

  // Returning parsed requested instances
  return requests;
}

std::vector<detectedInstance_t> detectInstances(HiCR::L1::InstanceManager &instanceManager)
{
  // Storage for the detected instances
  std::vector<detectedInstance_t> detectedInstances;
  
  // Getting the pointer to our own (coordinator) instance
  auto coordinator = instanceManager.getCurrentInstance();

  // Obtaining each of the instances HiCR topology
  for (const auto &instance : instanceManager.getInstances())
  if (instance->getId() != coordinator->getId())
  {
    // Storage for the newly detected instance
    detectedInstance_t detectedInstance;

    // Assigning instance pointer
    detectedInstance.instance = instance;

    // Running the RPC that obtains the instance's serialized topology
    instanceManager.execute(*instance, "Get Topology");

    // Gathering the return value
    auto returnValue = instanceManager.getReturnValue(*instance);

    // Receiving raw serialized topology information from the worker
    std::string serializedTopology = (char *)returnValue->getPointer();

    // Parsing serialized raw topology into a json object
    auto topologyJson = nlohmann::json::parse(serializedTopology);

    // Obtaining the host topology from the serialized object
    detectedInstance.topology.merge(HiCR::backend::host::hwloc::L1::TopologyManager::deserializeTopology(topologyJson));

    // Obtaining the ascend topology (if enabled at compilation time)
    #ifdef _HICR_USE_ASCEND_BACKEND_
    detectedInstance.topology.merge(HiCR::backend::ascend::L1::TopologyManager::deserializeTopology(topologyJson));
    #endif // _HICR_USE_ASCEND_BACKEND_

    // Adding detected instance to the storage
    detectedInstances.push_back(detectedInstance);
  }

  // Returning detected instances
  return detectedInstances;
}


void coordinatorFc(HiCR::L1::InstanceManager &instanceManager, const std::string& machineModelFilePath)
{
  // Getting the pointer to our own (coordinator) instance
  auto coordinator = instanceManager.getCurrentInstance();

  // Reading from machine model file
  std::string machineModelRaw;
  auto status = loadStringFromFile(machineModelRaw, machineModelFilePath);
  if (status == false) {  fprintf(stderr, "could not read from machine model file: '%s'\n", machineModelFilePath.c_str()); finalizeExecution(instanceManager, -1); }

  // Parsing received machine model file
  nlohmann::json machineModelJson;
  try { machineModelJson = nlohmann::json::parse(machineModelRaw); }
   catch (const std::exception& e)
    {  fprintf(stderr, "could not parse JSON from machine model file: '%s'. Reason: '%s'\n", machineModelFilePath.c_str(), e.what()); finalizeExecution(instanceManager, -1); }

  // Creating machine model to handle the instance creation and task execution
  HiCR::MachineModel machineModel(instanceManager);

  // Parsing the machine model into a request vector. Here the vector implies ordering, which allows the user specify which instances need to be allocated first
  std::vector<HiCR::MachineModel::request_t> requests;
  try { requests = machineModel.parseMachineModel(machineModelJson); }
   catch (const std::exception& e)
    {  fprintf(stderr, "Error while parsing the machine model. Reason: '%s'\n", e.what());  finalizeExecution(instanceManager, -1); }

  // Execute requests by finding or creating an instance that matches their topology requirements
  try { machineModel.executeRequests(requests); }
   catch (const std::exception& e)
    {  fprintf(stderr, "Error while executing requests. Reason: '%s'\n", e.what());  finalizeExecution(instanceManager, -1); }

  // Running the assigned task id in the correspondng instance
  for (auto &r : requests)
   for (auto &in : r.instances)
    instanceManager.execute(*in, r.taskName);

  // Now waiting for return values to arrive
  for (auto &r : requests)
   for (auto &in : r.instances)
    {
      // Getting return value as a memory slot
      auto returnValue = instanceManager.getReturnValue(*in);

      // Printing return value
      printf("[Coordinator] Received from instance %lu: '%s'\n", in->getId(), (const char*)returnValue->getPointer());
    }

  // Finalizing execution for all instances
  finalizeExecution(instanceManager);
}
