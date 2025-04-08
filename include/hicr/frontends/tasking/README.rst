.. _tasking:

***********************
Tasking
***********************

The tasking frontend contains building blocks to develop a task-based runtime system:

* :code:`task.hpp`       : The tasks to be executed
* :code:`callbackMap.hpp`: Map that relates task-related callbacks
* :code:`worker.hpp`     : A worker takes on tasks to execute

Task
------------------------

The tasks are build upon HiCR execution states.

To initialize a task one has to choose one of our compute managers (e.g. Pthreads).
This compute manager is then used to construct the execution unit which holds the function we want to execute.

..  code-block:: C++

    // Choose a compute manager (e.g. Pthreads)
    HiCR::backend::pthreads::ComputeManager cm;

    // Initialize the execution unit with the function (e.g. Boost)
    auto executionUnit = cm.createExecutionUnit([](){ printf("Hello, I am a Task\n"); });

    // Create a task
    auto task = new Task(executionUnit);

After that,  one has to create the execution state with the compute manager.

..  code-block:: C++

    // Initialize the execution state with the execution unit
    auto executionState = cm.createExecutionState(executionUnit);

    // Initialize the task with the execution state
    task.initialize(executionState);

    // Run the task
    task.run();

The following methods are present in a task:

* :code:`run()`    : This function starts running a task.
* :code:`suspend()`: This function yields the execution of the task, and returns to the worker's context.

Additionally, It provides basic support for stateful tasks with settable callbacks to notify when the task changes state:

* :code:`onTaskExecute`: Triggered as the task starts or resumes execution
* :code:`onTaskSuspend`: Triggered as the task is preempted into suspension
* :code:`onTaskFinish` : Triggered as the task finishes execution
* :code:`onTaskSync`   : Triggered as the task receives a sync signal (used for mutual exclusion mechanisms)

The CallbackMap class maps the state of any classes (e.g. task) to their corresponding callback. 

Worker
------------------------

The workers are build upon HiCR processing units.
The worker class also contains a simple loop that calls a :code:`_pullFunction()` , i.e.,
a user-defined scheduling function that should return the next task to execute (or a null pointer, if none is available).

Instead of initializing a task directly one can create a worker running those tasks.
This time we will use the :ref:`nOS-V backend`

..  code-block:: C++

    // Initialize the two compute managers
    HiCR::backend::nosv::ComputeManager processingUnitComputeManager;
    HiCR::backend::nosv::ComputeManager executionStateComputeManager;

    // Create the worker
    auto worker = Worker(&executionStateComputeManager, &processingUnitComputeManager);

After this we add as many processing units as wanted.

.. note::

    One would normaly link one processing unit but we kept it flexible.

..  code-block:: C++

    // Creating a Processing Unit of the first available computeResource
    auto ProcessingUnit = processingUnitComputeManager.createProcessingUnit(computeResources[0]);

    // Add the PU to the worker
    worker.addProcessingUnit(ProcessingUnit)

This worker is now ready to get tasks.

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
