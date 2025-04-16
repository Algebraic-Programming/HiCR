.. _kernel execution:

Kernel Execution
=====================

This is a set of simple examples showcasing how the HiCR API can be used to execute kernels or functions in multiple available processing (CPU, GPU, NPU, etc) using the same API. The code is structured as follows:

* :code:`source/` contains the different variants of this example corresponding to different backends

    * :code:`ascend.cpp` corresponds to the :ref:`ascend backend` backend implementation. The available kernel is compiled for Ascend 910. Run :code:`kernels/compile.sh` to call ATC and compile the kernel.
    * :code:`opencl.cpp` corresponds to the :ref:`opencl backend` backend implementation. The available kernel is compiled for OpenCL.
    * :code:`nosv.cpp` corresponds to the :ref:`nosv backend` backend implementation
    * :code:`pthreads.cpp` corresponds to the :ref:`pthreads backend` backend implementation

All the examples perform a GEMM operation, and follow the same pattern:

Identify Compute Topology
----------------------------

The first step is to discover the local compute topology, to find out the available compute devices and use one of them for running the example.

.. code-block:: C++

  // Asking backend to check the available devices
  auto topology = topologyManager.queryTopology();

  // Getting first device found in the topology
  auto device = *topology.getDevices().begin();

  // Getting compute resources
  auto computeResources = device->getComputeResourceList();

  // Selecting the first compute resource found
  auto firstComputeResource = *computeResources.begin();

Creating an execution state
----------------------------

The HiCR Core API defines execution units as a abstract description of a function or kernel for any kind of device. In the :code:`pthreads.cpp` example, we use a lambda function to describe work to be performed in the CPU and create an execution unit around it:

.. code-block:: C++

  auto executionUnit = computeManager.createExecutionUnit([=](void *arg) {
    gemm((double *)input1Host->getPointer(),
         (double *)input2Host->getPointer(),
         (double *)input3Host->getPointer(),
         (double *)alphaHost->getPointer(),
         (double *)betaHost->getPointer());
  });


The execution unit can be instantiated many times into an execution state object. Each execution state will hold its own particular current state of execution of the execution unit. We invoke the compute manager to create a new execution state:

.. code-block:: C++

  auto executionState = computeManager.createExecutionState(executionUnit);

Creating a processing unit
----------------------------

A processing unit represents an active compute resource than can be employed to run an execution state to its completion. The example creates one such processing unit out of the first available CPU core, as found by the topology manager.

.. code-block:: C++

  auto processingUnit = computeManager.createProcessingUnit(firstComputeResource);

After its creation, the processing unit is initialized (this creates and starts the corresponding pthread)

.. code-block:: C++

  computeManager.initialize(processingUnit);

Execution and Completion
--------------------------

To run the execution state, we assign it to the processing unit via the :code:`start` function:

.. code-block:: C++

  computeManager.start(processingUnit, executionState);

And then wait for completion with the :code:`await` function:

.. code-block:: C++

  computeManager.await(processingUnit);

The expected result of running this example with M = 4, N = 2, K = 8 is:

.. code-block:: bash

    First matrix [M, K]
    1.0 ...

    Second matrix [K, N]
    1.0 ... 

    Third matrix [M, N]
    1.0 ... 

    Output matrix [M, N]
    9.0 ...