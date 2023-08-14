/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

#pragma once

#include <set>
#include <memory>

#include <hicr/common/logger.hpp>
#include <hicr/dispatcher.hpp>
#include <hicr/memorySpace.hpp>

class Worker;

namespace HiCR
{

// Definition for resource unique identifiers
typedef uint64_t resourceId_t;

// Definition for function to run at resource
typedef std::function<void(void)> resourceFc_t;

/**
 * A compute resource
 */
class Resource
{
 friend class Worker;

 public:

 virtual ~Resource() = default;
 inline resourceId_t getId() { return _id; }

 /**
  * Returns the memory space associated with this compute resource.
  *
  * This refers to addressable main memory.
  */
 virtual MemorySpace& getMemorySpace() = 0;

 protected:

 Resource(const resourceId_t id) : _id(id) {};
 virtual void initialize() = 0;
 virtual void run(resourceFc_t fc) = 0;
 virtual void finalize() = 0;
 virtual void await() = 0;

 // Unique local identifier for the resource
 resourceId_t _id;

 // Copy of a function to run
 resourceFc_t _fc;
};

typedef std::vector<std::unique_ptr<Resource>> resourceList_t;

} // namespace HiCR

