.. _tasking:

***********************
Tasking
***********************

The tasking frontend contains building blocks to develop a task-based runtime system:

* :code:`task`       : The tasks to be executed
* :code:`callbackMap`: Map that relates task-related callbacks
* :code:`worker`     : A worker takes on tasks to execute

Task and CallbackMap
=====================

The tasks are built upon HiCR execution states.

Task creation
---------------

To create and initialize a task one has to choose one of our compute managers (e.g. Pthreads).
This compute manager is then used to construct an execution unit which holds the function we want to execute.

..  code-block:: C++

    // Choose a compute manager (e.g. Pthreads)
    HiCR::backend::pthreads::ComputeManager cm;

    // Initialize the execution unit with the function (e.g. Boost)
    auto executionUnit = cm.createExecutionUnit([](){ printf("Hello, I am a Task\n"); });

    // Create a task
    auto task = Task(executionUnit);


.. _task-run:

Task run and suspend
---------------------

After that, one has to create an execution state with the compute manager and run the task

..  code-block:: C++

    // Initialize the execution state with the execution unit
    auto executionState = cm.createExecutionState(executionUnit);

    // Initialize the task with the execution state
    task.initialize(executionState);

    // Run the task
    task.run();

    // This function yields the execution of the task (if supported), and returns to the worker's context
    task.suspend()



Additionally, It provides basic support for stateful tasks with settable callbacks to notify when the task changes state:

* :code:`onTaskExecute`: Triggered as the task starts or resumes execution
* :code:`onTaskSuspend`: Triggered as the task is preempted into suspension
* :code:`onTaskFinish` : Triggered as the task finishes execution
* :code:`onTaskSync`   : Triggered as the task receives a sync signal (used for mutual exclusion mechanisms)

The CallbackMap class maps the state of any classes (e.g. task) to their corresponding callback. 

Worker
==========

The workers are built upon HiCR processing units.
The worker class also contains a simple loop that calls a user-defined scheduling function.
The function returns the next task to execute.

Worker creation
==================

Instead of running a task directly (see :ref:`task-run`), one can create a worker running those tasks.
This time we will use the :ref:`nosv backend` backend. After the creation, we need to add processing units to the
tasks.

..  code-block:: C++

    // Initialize the two compute managers
    HiCR::backend::nosv::ComputeManager processingUnitComputeManager;
    HiCR::backend::nosv::ComputeManager executionStateComputeManager;

    // Create the worker
    auto worker = Worker(&executionStateComputeManager, &processingUnitComputeManager);

    // Creating a Processing Unit of the first available computeResource
    auto processingUnit = processingUnitComputeManager.createProcessingUnit(computeResources[0]);

    // Add the PU to the worker
    worker.addProcessingUnit(processingUnit)

This worker is now ready to get tasks.

Worker usage
=================

To use a worker, the following lifecycle should be followed:

.. code-block:: c++

    // Initializes the worker and its resources
    worker.initialize();

    // Initializes the worker's task execution loop
    worker.start();

    // Suspends the execution of the underlying resource(s)
    worker.suspend();

    // Terminates the worker's task execution loop
    worker.terminate();

    // A function that will suspend the execution of the caller until the worker has stopped
    worker.await(); 


Additionally, these are the following states a worker can be:

* :code:`uninitialized`: The worker object has been instantiated but not initialized
* :code:`ready`        : The worker has been ininitalized (or is back from executing) and can currently run
* :code:`running`      : The worker has started executing
* :code:`suspending`   : The worker is in the process of being suspended
* :code:`suspended`    : The worker has suspended
* :code:`resuming`     : The worker is in the process of being resumed
* :code:`terminating`  : The worker has been issued for termination (but still running)
* :code:`terminated`   : The worker has terminated
