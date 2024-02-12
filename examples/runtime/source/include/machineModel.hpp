#pragma once

#include <hicr/L0/topology.hpp>
#include <frontends/machineModel/machineModel.hpp>
#include <backends/host/L0/device.hpp>
#include <backends/host/L0/memorySpace.hpp>
#include <backends/host/L0/computeResource.hpp>
#include <memory>
#include <fstream>
#include <sstream>

// Taken from https://stackoverflow.com/questions/116038/how-do-i-read-an-entire-file-into-a-stdstring-in-c/116220#116220
inline std::string slurp(std::ifstream &in)
{
  std::ostringstream sstr;
  sstr << in.rdbuf();
  return sstr.str();
}

// Loads a string from a given file
inline bool loadStringFromFile(std::string &dst, const std::string fileName)
{
  std::ifstream fi(fileName);

  // If file not found or open, return false
  if (fi.good() == false) return false;

  // Reading entire file
  dst = slurp(fi);

  // Closing file
  fi.close();

  return true;
}

HiCR::L0::Topology parseTopology(const nlohmann::json &topologyJson)
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

#ifdef _HICR_USE_HWLOC_BACKEND_

  // Creating list of memory spaces (only one, with the total host memory)
  HiCR::L0::Device::memorySpaceList_t memorySpaces({std::make_shared<HiCR::backend::host::L0::MemorySpace>(hostRamSize)});

  // Creating the CPU (NUMA Domain) device type with the assigned number of cores
  auto hostDevice = std::make_shared<HiCR::backend::host::L0::Device>(0, computeResources, memorySpaces);

  // Adding host device to the topology
  topology.addDevice(hostDevice);

#endif

  // Returning the HiCR-formatted topology
  return topology;
}

/**
 * This function takes a valid JSON-based description of a machine model and parses it into HiCR request objects that can be
 * satisfied later by creating a new instance using the instance manager
 *
 * @param[in] machineModelJson The machine model to parse, in JSON format
 * @return A vector of requests, in the same order as provided in the JSON file
 */
std::vector<HiCR::MachineModel::request_t> parseMachineModel(const nlohmann::json &machineModelJson)
{
  // Storage for the parsed instances from the machine model
  std::vector<HiCR::MachineModel::request_t> requests;

  // Checking for correct format in the machine model
  if (machineModelJson.contains("Instances") == false) throw std::runtime_error("the machine model does not contain an 'Instances' entry\n");
  if (machineModelJson["Instances"].is_array() == false) throw std::runtime_error("The 'Instances' entry in the machine model is not an array\n");

  // Now iterating over all requested instances
  for (const auto &instance : machineModelJson["Instances"])
  {
    // Storage for the new requested instance to add
    HiCR::MachineModel::request_t newRequestedInstance;

    // Parsing task name
    if (instance.contains("Entry Point") == false) throw std::runtime_error("the requested instance does not contain a 'Entry Point' entry\n");
    if (instance["Entry Point"].is_string() == false) throw std::runtime_error("The instance 'Entry Point' entry is not a string\n");
    newRequestedInstance.entryPointName = instance["Entry Point"].get<std::string>();

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

/*
 * This function allows us to customize the criteria that determine whether the detected topology satisfies one of our requests
 */
bool isTopologyAcceptable(const HiCR::L0::Topology &a, const HiCR::L0::Topology &b)
{
  // For this example, it suffices that topology B has more or equal:
  //  + Total Core Count (among all NUMA domains)
  //  + Total RAM size (among all NUMA domains)
  // than topology A.

  size_t tACoreCount = 0;
  size_t tBCoreCount = 0;

  size_t tAMemSize = 0;
  size_t tBMemSize = 0;

  // Processing topology A
  for (const auto &d : a.getDevices())
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
  }

  // Processing topology B
  for (const auto &d : b.getDevices())
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
  }

  // Evaluating criteria
  if (tACoreCount > tBCoreCount) return false;
  if (tAMemSize > tBMemSize) return false;

  // If no criteria failed, return true
  return true;
}

std::vector<HiCR::MachineModel::request_t> loadMachineModelFromFile(const std::string& machineModelFile)
{
  // Reading from machine model file
  std::string machineModelRaw;
  auto status = loadStringFromFile(machineModelRaw, machineModelFile);
  if (status == false)
  {
    fprintf(stderr, "could not read from machine model file: '%s'\n", machineModelFile.c_str());
    HiCR::Runtime::abort(-1);
  }

  // Parsing received machine model file
  nlohmann::json machineModelJson;
  try
  {
    machineModelJson = nlohmann::json::parse(machineModelRaw);
  }
  catch (const std::exception &e)
  {
    fprintf(stderr, "could not parse JSON from machine model file: '%s'. Reason: '%s'\n", machineModelFile.c_str(), e.what());
    HiCR::Runtime::abort(-1);
  }

  // Parsing the machine model into a request vector. Here the vector implies ordering, which allows the user specify which instances need to be allocated first
  std::vector<HiCR::MachineModel::request_t> requests;
  try
  {
    requests = parseMachineModel(machineModelJson);
  }
  catch (const std::exception &e)
  {
    fprintf(stderr, "Error while parsing the machine model. Reason: '%s'\n", e.what());
    HiCR::Runtime::abort(-1);
  }

  return requests;
}