#pragma once

#include <set>

#include <hicr/common.hpp>
#include <hicr/taskPool.hpp>

namespace HiCR
{

class Resource
{
 public:

 Resource(const resourceId_t id) : _id(id) {};
 virtual ~Resource() = default;

 virtual void initialize() = 0;
 virtual void finalize() = 0;
 virtual void await() = 0;

 inline resourceId_t getId() { return _id; }
 void subscribe(TaskPool* pool) { _pools.insert(pool); }

 protected:

 // Unique local identifier for the resource
 resourceId_t _id;

 // Task pools that this resource is subscribed to
 std::set<TaskPool*> _pools;
};

typedef std::vector<Resource*> resourceList_t;

} // namespace HiCR
