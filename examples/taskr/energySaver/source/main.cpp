#include <cstdio>
#include <cstring>
#include <taskr.hpp>
#include <hicr/backends/sharedMemory/sharedMemory.hpp>

#define WORK_TASK_COUNT 1000

void work()
{
  __volatile__ double value = 2.0;
  for (size_t i = 0; i < 5000; i++)
    for (size_t j = 0; j < 5000; j++)
    {
      value = sqrt(value + i);
      value = value * value;
    }
}

void wait()
{
  // Reducing maximum active workers to 1
  taskr::setMaximumActiveWorkers(1);

  printf("Starting long task...\n");
  fflush(stdout);
  sleep(5.0);
  printf("Finished long task...\n");
  fflush(stdout);

  // Increasing maximum active workers
  taskr::setMaximumActiveWorkers(1024);
}

int main(int argc, char **argv)
{
  // Initializing Pthreads backend to run in parallel
  auto t = new HiCR::backend::sharedMemory::SharedMemory();

  // Initializing taskr
  taskr::initialize(t);

  printf("Starting many work tasks...\n");
  fflush(stdout);

  for (size_t i = 0; i < WORK_TASK_COUNT; i++)
  {
    auto workTask = new taskr::Task(i, &work);
    taskr::addTask(workTask);
  }

  auto waitTask = new taskr::Task(WORK_TASK_COUNT + 1, wait);
  for (size_t i = 0; i < WORK_TASK_COUNT; i++) waitTask->addTaskDependency(i);
  taskr::addTask(waitTask);

  for (size_t i = 0; i < WORK_TASK_COUNT; i++)
  {
    auto workTask = new taskr::Task(WORK_TASK_COUNT + i + 1, &work);
    workTask->addTaskDependency(WORK_TASK_COUNT + 1);
    taskr::addTask(workTask);
  }

  // Running taskr
  taskr::run();

  // Finalizing taskr
  taskr::finalize();

  printf("Finished all tasks.\n");
  fflush(stdout);

  return 0;
}
