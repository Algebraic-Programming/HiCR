.. _tasking:

***********************
Tasking
***********************

Here we provide a simple runtime system for HiCR's tasking frontend.

This runtime system is kept to its minimal.

In the following, we will explain the important steps to set up the runtime system of HiCR's tasking.
After that, we will go through the examples.

Initializing the runtime
------------------------

The first step is to initialize the runtime itself. For that one needs to use two compute managers. 
One for the Execution States (tasks) and one for the ProcessingUnits (workers).

In the current version of HiCR, the following two configurations are acceptable ComputeManagers:

1. Boost + pthreads

..  code-block:: C++

    HiCR::backend::boost::L1::ComputeManager    executionStateComputeManager;
    HiCR::backend::pthreads::L1::ComputeManager processingUnitComputeManager;

2. nOS-V

..  code-block:: C++

    HiCR::backend::nosv::L1::ComputeManager executionStateComputeManager;
    HiCR::backend::nosv::L1::ComputeManager processingUnitComputeManager;

After the chosen compute managers one can initialize the runtime:

..  code-block:: C++

    // Initializing the runtime with chosen ComputeManagers
    Runtime runtime(&executionStateComputeManager, &processingUnitComputeManager);

Adding a Processing Unit
------------------------

The next step is to add as many processing unit as wanted. Each processing unit is linked to a worker.

..  code-block:: C++

    // Creating a Processing Unit of the first available computeResource
    auto ProcessingUnit = processingUnitComputeManager.createProcessingUnit(computeResources[0]);

    // adding available compute resources
    runtime.addProcessingUnit(ProcessingUnit);


Adding tasks
------------------------

After the tasking workers have been initialized, the next step is to create the tasks.
For this runtime system, each task consists of an unique label and a function which it has to execute.
The label is an identifier for when this task has to be executed.

..  code-block:: C++

    // Create a new tasks
    uint64_t label = 0;
    auto fc = [](void *arg){ printf("Hello, I am Task: %lu\n", ((Task *)arg)->getLabel()); };

    auto MyTask = new Task(label,  fc);

    runtime.addTask(MyTask);

Executing tasks
------------------------

Finally, we can let the runtime execute.

..  code-block:: C++
    // Running tasks
    runtime.run();

    // Done from here, save to do other things