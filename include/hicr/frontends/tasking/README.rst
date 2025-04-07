.. _tasking:

***********************
Tasking
***********************

The tasking frontend contains building blocks to develop a task-based runtime system:

* :code:`worker.hpp`     : A worker takes on tasks to execute
* :code:`task.hpp`       : The tasks to be executed
* :code:`callbackMap.hpp`: Map that relates task-related callbacks

worker.hpp
------------------------

The workers are build apon HiCR Processing Units.
The worker class also contains a simple loop that calls a pullfunction, i.e.,
a user-defined scheduling function that should return the next task to execute (or a null pointer, if none is available).

The following methods are present in a worker:

* :code:`initialize()`: Initializes the worker and its resources
* :code:`start()`     : Initializes the worker's task execution loop
* :code:`suspend()`   : Suspends the execution of the underlying resource(s)
* :code:`terminate()` : Terminates the worker's task execution loop
* :code:`await()`     : A function that will suspend the execution of the caller until the worker has stopped

Additionally, these are the following states a worker can be:

* :code:`uninitialized`: The worker object has been instantiated but not initialized
* :code:`ready`        : The worker has been ininitalized (or is back from executing) and can currently run
* :code:`running`      : The worker has started executing
* :code:`suspending`   : The worker is in the process of being suspended
* :code:`suspended`    : The worker has suspended
* :code:`resuming`     : The worker is in the process of being resumed
* :code:`terminating`  : The worker has been issued for termination (but still running)
* :code:`terminated`   : The worker has terminated

task.hpp
------------------------

The tasks are build apon HiCR Execution States.

The following methods are present in a task:

* :code:`run()`    : This function starts running a task.
* :code:`suspend()`: This function yields the execution of the task, and returns to the worker's context.

Additionally, It provides basic support for stateful tasks with settable callbacks to notify when the task changes state:

* :code:`onTaskExecute`: Triggered as the task starts or resumes execution
* :code:`onTaskSuspend`: Triggered as the task is preempted into suspension
* :code:`onTaskFinish` : Triggered as the task finishes execution
* :code:`onTaskSync`   : Triggered as the task receives a sync signal (used for mutual exclusion mechanisms)

callbackMap.hpp
------------------------

The CallbackMap class maps the state of any classes (e.g. task) to their corresponding callback. 