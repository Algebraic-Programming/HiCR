.. _topology distributed:

Topology: Distributed
=====================

This example uses RPC to enable a coordinator instance to get the topology of all the other active instances. The :ref:`rpcengine`


This example showcases how the abstract HiCR Core API can be used to discover compute and memory devices in the system. The code is structured as follows:

* :code:`source/` contains the different variants of this example corresponding to different backends

    * :code:`mpi.cpp` corresponds to the :ref:`mpi backend` backend implementation. It uses hwloc to discover local resources and then communicates them via MPI 

Local Topology Discovery
------------------------

Each Instance discovers its own local topology

.. code-block:: c++

    // Creating HWloc topology object
    hwloc_topology_t topology;

    // Reserving memory for hwloc
    hwloc_topology_init(&topology);

    // Initializing hwloc (CPU) topology manager
    HiCR::backend::hwloc::L1::TopologyManager tm(&topology);

    // Gathering topology from the topology manager
    const auto t = tm.queryTopology();

Topology RPC registration
-------------------------

Each Instance instantiates the :code:`RPCEngine` and registers and RPC to serialize and send its local topology to the caller

.. code-block:: c++

    // Creating RPC engine instance
    HiCR::frontend::RPCEngine rpcEngine(...);

    // Initialize RPC engine
    rpcEngine.initialize();

    // Creating execution unit to run as RPC
    auto executionUnit = std::make_shared<HiCR::backend::pthreads::L0::ExecutionUnit>([&](void *closure) { sendTopology(rpcEngine); });

    // Adding RPC target by name and the execution unit id to run
    rpcEngine.addRPCTarget(TOPOLOGY_RPC_NAME, executionUnit);

Listening for RPCs
------------------

All the instances except Root listens for incoming RPC requests

.. code-block:: c++

    // Listening for RPC requests
    rpcEngine.listen();

RPC Invokation
--------------

The Root instance requests the topology from all the other instances, merge them, and then displays them

.. code-block:: c++

    // Getting instance manager from the rpc engine
    auto im = rpcEngine.getInstanceManager();

    // Querying instance list
    auto &instances = im->getInstances();

    // Getting the pointer to our own (coordinator) instance
    auto coordinator = im->getCurrentInstance();

    // Invoke RPC
    for (const auto &instance : instances)
      if (instance->getId() != coordinator->getId()) rpcEngine.requestRPC(*instance, TOPOLOGY_RPC_NAME);

    // Getting return values from the RPCs containing each of the worker's topology
    for (const auto &instance : instances)
      if (instance == coordinator)
      {
        // Getting return value as a memory slot
        auto returnValue = rpcEngine.getReturnValue(*instance);

        // Receiving raw serialized topology information from the worker
        std::string serializedTopology = (char *)returnValue->getPointer();

        // Parsing serialized raw topology into a json object
        auto topologyJson = nlohmann::json::parse(serializedTopology);

        // Freeing return value
        rpcEngine.getMemoryManager()->freeLocalMemorySlot(returnValue);

        // HiCR topology object to obtain
        HiCR::L0::Topology topology;

        // Merge topologies
        topology.merge(HiCR::backend::hwloc::L1::TopologyManager::deserializeTopology(topologyJson));
        }
    }

The result should look like the following:

.. code-block:: bash

    * Worker 1 Topology:
      + 'NUMA Domain'
        Compute Resources: 44 Processing Unit(s)
        Memory Space:     'RAM', 93.071026 Gb
      + 'NUMA Domain'
        Compute Resources: 44 Processing Unit(s)
        Memory Space:     'RAM', 94.437321 Gb
    * Worker 2 Topology:
      + 'NUMA Domain'
        Compute Resources: 44 Processing Unit(s)
        Memory Space:     'RAM', 93.071026 Gb
      + 'NUMA Domain'
        Compute Resources: 44 Processing Unit(s)
        Memory Space:     'RAM', 94.437321 Gb
    * Worker 3 Topology:
      + 'NUMA Domain'
        Compute Resources: 44 Processing Unit(s)
        Memory Space:     'RAM', 93.071026 Gb
      + 'NUMA Domain'
        Compute Resources: 44 Processing Unit(s)
        Memory Space:     'RAM', 94.437321 Gb