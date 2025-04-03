.. _rpc engine:

RPC Engine
=====================

This example showcases the RPC Engine capabilities. The code is structured as follows:

* :code:`source/` contains the different variants of this example corresponding to different backends

    * :code:`mpi.cpp` corresponds to the :ref:`mpi backend` backend implementation
   
First, we use the topology manager to query the local HiCR instance's topology

.. code-block:: C++

   auto topology = topologyManager.queryTopology();

Then, we get the first available compute resource and memory space of the first device: 

.. code-block:: C++

  // Selecting first device
  auto d = *topology.getDevices().begin();

  // Get first memory space for RPC buffers
  auto memSpaces = d->getMemorySpaceList();
  auto bufferMemorySpace = *memSpaces.begin();
  
  // Grabbing first compute resource for computing RPCs
  auto computeResources = d->getComputeResourceList();
  auto executeResource = *computeResources.begin(); 

Then, create an RPC Engine define a lambda, and associate it with an RPC:

.. code-block:: C++

   // Creating RPC engine instance
  HiCR::frontend::RPCEngine rpcEngine(...);

  // Initialize RPC engine
  rpcEngine.initialize();

  // Creating execution unit to run as RPC
  auto rpcExecutionUnit =
    std::make_shared<HiCR::backend::pthreads::L0::ExecutionUnit>([&im](void *closure) { 
      printf("Instance %lu: running Test RPC\n", im->getCurrentInstance()->getId()); 
    });
  
  // Registering RPC to listen to
  rpcEngine.addRPCTarget("Test RPC", rpcExecutionUnit);

The RPC is invoked by the root instance, the other instances listen to incoming RPCs:

.. code-block:: c++

 if (currentInstance->isRootInstance())
  {
    for (auto &instance : im.getInstances())
      if (instance != currentInstance) rpcEngine.requestRPC(*instance, "Test RPC");
  }
  else
    rpcEngine.listen();

The result should look like the following:

.. code-block:: bash

  Instance 1: running Test RPC
  Instance 2: running Test RPC
  Instance 4: running Test RPC
  Instance 3: running Test RPC
  Instance 5: running Test RPC
