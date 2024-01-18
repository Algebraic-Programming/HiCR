#pragma once

#define PROCESSING_UNIT_ID 0
#define TOPOLOGY_RPC_ID 0
#define TOPOLOGY_RPC_NAME "Get Topology"

#include <nlohmann_json/json.hpp>

// Enabling topology managers (to discover the system's hardware) based on the selected backends during compilation

#ifdef _HICR_USE_ASCEND_BACKEND_
  #include <acl/acl.h>
  #include <backends/ascend/L1/topologyManager.hpp>
#endif // _HICR_USE_ASCEND_BACKEND_

#ifdef _HICR_USE_HWLOC_BACKEND_
  #include <hwloc.h>
  #include <backends/host/hwloc/L1/topologyManager.hpp>
#endif // _HICR_USE_HWLOC_BACKEND_
