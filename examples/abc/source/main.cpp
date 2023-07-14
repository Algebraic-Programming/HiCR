#include <cstring>
#include <cstdio>
#include <taskr.hpp>

int main( int argc, char** argv )
{
 // Initializing taskr
 taskr::initialize();

 for (size_t i = 0; i < 10; i++)
 {
  auto cTask = new taskr::Task(i*3 + 2, [i] () { printf("Task C%lu\n", i); fflush(stdout); });
  cTask->addTaskDependency(i*3 + 1);
  taskr::addTask(cTask);
 }

 for (size_t i = 0; i < 10; i++)
 {
  auto bTask = new taskr::Task(i*3 + 1, [i] () { printf("Task B%lu\n", i); fflush(stdout); });
  bTask->addTaskDependency(i*3 + 0);
  taskr::addTask(bTask);
 }

 for (size_t i = 0; i < 10; i++)
 {
  auto aTask = new taskr::Task(i*3 + 0, [i] () { printf("Task A%lu\n", i); fflush(stdout); });
  if (i > 0) aTask->addTaskDependency(i*3 - 1);
  taskr::addTask(aTask);
 }

 // Running taskr
 taskr::run();

 // Finalizing taskr
 taskr::finalize();

 return 0;
}
