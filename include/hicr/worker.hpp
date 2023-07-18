#pragma once

#include <hicr/dispatcher.hpp>
#include <hicr/resource.hpp>
#include <hicr/task.hpp>

namespace HiCR {

enum workerState_t { t_uninitialized, t_initialized, t_started, t_finishing };

class Worker
{
 private:

 // Worker state
 workerState_t _state = t_uninitialized;

 // Dispatchers that this resource is subscribed to
 std::set<Dispatcher*> _dispatchers;

 // Group of resources the worker can freely use
 std::vector<Resource*> _resources;

 void mainLoop()
 {
  while(_state == t_started)
  {
   for (auto dispatcher : _dispatchers)
   {
    // Attempt to both pop and pull from dispatcher
    auto task = dispatcher->pull();

    // If a task was returned, then execute it
    if (task != NULL) task->run();
   }
  }
 }

 public:

 Worker() = default;
 ~Worker() = default;

 void initialize()
 {
  // Checking state
  if (_state != t_uninitialized) LOG_ERROR("Attempting to initialize already initialized worker");

  // Checking we have at least one assigned resource
  if (_resources.empty()) LOG_ERROR("Attempting to initialize worker without any assigned resources");

  // Initializing all resources
  for (auto r : _resources) r->initialize();

  // Transitioning state
  _state = t_initialized;
 }

 void start()
 {
  // Checking state
  if (_state != t_initialized) LOG_ERROR("Attempting to start worker that is not in the 'initialized' state");

  // Checking we have at least one assigned resource
  if (_resources.empty()) LOG_ERROR("Attempting to start worker without any assigned resources");

  // Transitioning state
  _state = t_started;

  // Launching worker in the lead resource
  _resources[0]->run([this](){ this->mainLoop();});
 }

 void stop()
 {
  // Checking state
  if (_state != t_started) LOG_ERROR("Attempting to stop worker that is not in the 'started' state");

  // Transitioning state
  _state = t_finishing;
 }

 void await()
 {
  // Checking state
  if (_state != t_finishing && _state != t_started) LOG_ERROR("Attempting to wait for a worker that is not in the 'started' or 'finishing' state");

  // Wait for the resource to free up
  _resources[0]->await();

  // Transitioning state
  _state = t_initialized;
 }

 void finalize()
 {
  // Checking state
  if (_state != t_initialized) LOG_ERROR("Attempting to finalize a worker that is not in the 'initialized' state");

  // Finalize all resources
  for (auto r : _resources) r->finalize();

  // Transitioning state
  _state = t_uninitialized;
 }

 void subscribe(Dispatcher* dispatcher) { _dispatchers.insert(dispatcher); }
 void addResource(Resource* resource) { _resources.push_back(resource); }
};

} // namespace HiCR

