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

#pragma once

#define TEST_RPC_PROCESSING_UNIT_ID 0
#define TEST_RPC_EXECUTION_UNIT_ID 0

#include <nlohmann_json/json.hpp>

// Enabling topology managers (to discover the system's hardware) based on the selected backends during compilation

#ifdef _HICR_USE_HWLOC_BACKEND_
  #include <hwloc.h>
  #include <backends/host/hwloc/L1/topologyManager.hpp>
#endif // _HICR_USE_HWLOC_BACKEND_
