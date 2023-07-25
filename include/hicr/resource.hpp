#pragma once

#include <set>
#include <memory>

#include <hicr/common/logger.hpp>
#include <hicr/dispatcher.hpp>

class Worker;

namespace HiCR
{

// Definition for resource unique identifiers
typedef uint64_t resourceId_t;

// Definition for function to run at resource
typedef std::function<void(void)> resourceFc_t;

class Resource
{
 friend class Worker;

 public:

 virtual ~Resource() = default;
 inline resourceId_t getId() { return _id; }

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
