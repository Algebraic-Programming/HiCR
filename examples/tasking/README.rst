.. _taskingExample:

***********************
Tasking
***********************

To utilize the HiCR tasking frontend, we provide a simple runtime system.

Runtime
========

This runtime system is kept minimal and in the following section, we will explain the steps to set up this runtime system for HiCR tasking.

Initializing the runtime
-----------------------------

The first step is to initialize the runtime itself. To do this, one needs to use two compute managers: 
one for the execution states (tasks) and one for the processing units (workers).

In the current version of HiCR, the following two configurations are acceptable ComputeManagers for this runtime:

* :ref:`pthreads backend` + :ref:`boost backend`: pthreads compute manager is for kernel-level thread-based workers, and Boost is for coroutine-based tasks. The main advantage of using this setup is the quick user-level context switching, especially for fine-grained tasks.

..  code-block:: C++

    HiCR::backend::pthreads::ComputeManager processingUnitComputeManager;
    HiCR::backend::boost::ComputeManager    executionStateComputeManager;

* :ref:`nosv backend`: The workers and tasks are wrapped in nOS-V tasks. nOS-V has the advantage of thread-level storage and co-execution of independent tasks. Coarse-grained tasks can have a benefit of it.

..  code-block:: C++

    HiCR::backend::nosv::ComputeManager processingUnitComputeManager;
    HiCR::backend::nosv::ComputeManager executionStateComputeManager;

After choosing the compute managers, one can initialize the runtime like this:

..  code-block:: C++

    // Initializing the runtime with chosen ComputeManagers
    Runtime runtime(&executionStateComputeManager, &processingUnitComputeManager);


Adding workers
------------------------

The next step is to add as many processing units as available of your system. Each processing unit is linked to one worker.

..  code-block:: C++

    // Creating a Processing Unit of the first available computeResource
    auto ProcessingUnit = processingUnitComputeManager.createProcessingUnit(computeResources[0]);

    // adding available compute resources
    runtime.addProcessingUnit(ProcessingUnit);


Adding tasks
------------------------

After the tasking workers have been initialized, the next step is to create the tasks.
For this runtime system, we modified the task class to also include a unique label for execution dependencies.
This label refers to the order in which the tasks have to be executed in this runtime system.

..  code-block:: C++

    // Create a new tasks
    uint64_t label = 0;
    auto fc = [](void *arg){ printf("Hello, I am Task: %lu\n", ((Task *)arg)->getLabel()); };

    auto MyTask = new Task(label,  fc);

    runtime.addTask(MyTask);

Executing tasks
------------------------

Finally, we can let the runtime execute the tasks.

..  code-block:: C++

    // Running tasks
    runtime.run();


Examples
=========

The following examples use the runtime:


.. toctree::
    :maxdepth: 4

    abcTasks/README
    fibonacci/README
    resourceList/README
  