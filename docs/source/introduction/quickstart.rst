.. _quickstart:

Quickstart Guides
##################

These guides explain how to develop a HiCR-based application using its Core API using simple examples. For using HiCR through its built-in frontends, see the :ref:`builtinFrontends` section.

.. warning::

    This guide is only intended as an overview of the basic aspects of HiCR use and may not exactly reflect the current state of its API or features. For up-to-date HiCR examples, see the :ref:`examples` section.

Instance Management 
***************************************

.. note::

    See Related Example: (TBD)


Instantiation
-----------------

Since HiCR is an include-only C++ library, it suffices that the relevant set of header files are included. Depending on the application and its choice of backends, only a subset of all header files need to be included. For example, to use the MPI-based instance manager backend, we need to include:

..  code-block:: C++

  #include <hicr/backends/mpi/L1/instanceManager.hpp>

To use a backend, a user needs to instantiate an L1 class implemented by the chosen backend by calling its constructor with the correct parameters (see: `C++ API Reference <../doxygen/html/annotated.html>`_). For example:

..  code-block:: C++

  ////// Backend-specific operations

  // Initializing MPI itself
  MPI_Init(&argc, &argv);

  // Invoking constructor with the correct parameters 
  HiCR::backend::mpi::L1::InstanceManager myInstanceManager(MPI_COMM_WORLD);

  /////// Application is implementation-agnostic from this point forward
  
Creates a new MPI-based instance manager whose initial communicator is :code:`MPI_COMM_WORLD`. The instantiation of an L1 class is the only backend-specific part of a HiCR application. After all L1 classes are created, the rest of the application remains backend-agnostic. This includes all methods in L1 classes, as well as the creation/detection/manipulation of L0 objects. 

Detecting instances
--------------------------

The following example shows how to detect the currently created instances:

..  code-block:: C++

  // Getting a list of HiCR instances from the instance manager
  const auto instances = myInstanceManager.getInstances();

  // Printing how many instances were created via 'mpirun'
  printf("Instance count: %lu\n", instances.size());

This example requests the list of HiCR instances (as initially created by MPI or any other instance manager) and prints the number of instances.

Instance Creation
----------------------

If the backend supports it, the instance manager can be used to request the creation of a new HiCR instance on runtime:

..  code-block:: C++

  // Attempting to create a new instance
  auto newInstance = myInstanceManager.createInstance();

New instances will run in an SPMD fashion, starting from the :code:`main` function. To distinguish the behavior of different instances, two mechanisms may be employed:

* *Instance ID*, an integer guaranteed to be unique for each instance. No other guarantees are offered.
* *Root Instance*. Regardless of the backend chosen, only a single HiCR instance is designated as root. This root instance is within the first set of instances created at launch time.

The following example shows how these mechanisms can be used to identify each HiCR instance:

..  code-block:: C++

  // Attempting to create a new instance
  auto instanceId = myInstanceManager.getCurrentInstance()->getId();
  auto isRootId = instanceId == myInstanceManager.getRootInstanceId();

  printf("Instance %u - I am %s the root instance\n", isRootId ? "" : "not");

Expected output:

..  code-block:: bash

  Instance 0 - I am the root instance
  Instance 1 - I am not the root instance
  ...
  Instance N - I am not the root instance

Topology Management
*******************************************

.. note::

    See Related Example: (TBD)

A programmer may discover the topology of the local system's devices by using backends that implement the :code:`HiCR::L1::TopologyManager` class. For example, the HWLoC backend may be used to discover the local CPU socket / core / processing unit distribution and their associated memory spaces.

Instantiating a Topology Manager
-----------------------------------

The following example shows how to instantiate the HWLoC topology manager:

..  code-block:: C++

    #include <hwloc.h>
    #include <hicr/backends/host/hwloc/L1/topologyManager.hpp>

    int main(int argc, char **argv)
    {
        // Creating HWloc topology object
        hwloc_topology_t topology;

        // Reserving memory for hwloc
        hwloc_topology_init(&topology);

        // Initializing HWLoc-based host (CPU) topology manager
        HiCR::backend::host::hwloc::L1::TopologyManager topologyManager(&topology);

        /////// Application is implementation-agnostic from this point forward
        ...
    }

Querying Topology
---------------------

After instantiating the topology manager class, it can be used to query the devices it has access to and print their compute and memory resources to screen:

..  code-block:: C++

  // Querying the devices that this topology manager can detect
  auto topology = topologyManager.queryTopology();

  // Now summarizing the devices seen by this topology manager
  for (const auto &d : topology.getDevices())
  {
    printf("  + '%s'\n", d->getType().c_str());
    printf("    Compute Resources: %lu %s(s)\n", d->getComputeResourceList().size(), (*d->getComputeResourceList().begin())->getType().c_str());
    for (const auto &m : d->getMemorySpaceList()) printf("    Memory Space:     '%s', %f Gb\n", m->getType().c_str(), (double)m->getSize() / (double)(1024ul * 1024ul * 1024ul));
  }

The expected result being:

..  code-block:: bash

  + 'NUMA Domain'
    Compute Resources: 44 Processing Unit(s)
    Memory Space:     'RAM', 94.483532 Gb
  + 'NUMA Domain'
    Compute Resources: 44 Processing Unit(s)
    Memory Space:     'RAM', 93.024166 Gb

It is important to point that the HWLoc backend will not discover other type of devices (e.g., GPU). For other devices, the appropriate backend should be used.

Serialization
-----------------

The resulted topologies may be joined together and serialized for sharing:

..  code-block:: C++

  // Querying the devices that multiple topology managers can detect
  auto topology1 = topologyManager1.queryTopology();
  auto topology2 = topologyManager2.queryTopology();

  // Mege both topologies into topology1
  topology1.merge(topology2)

  // Serialize the resulting topolog into a JSON serialized object for sending to report to, for example, a remote instance
  auto serializedTopology = topology1.serialize();

Data Management
*******************************************

.. note::

    See Related Example: (TBD)

HiCR uses Memory Slots to represent contiguous segments of memory upon which allocation, deallocation and data motion operations can be made. This abstraction enables different backends to represent allocations on devices other than the system's RAM (e.g., GPU RAM), or whose addressing do not correspond to a number (e.g., a port on a network device). 

Memory Allocation
-------------------

Memory slots allocated by the currently running HiCR instance are deemed *local*. The allocation is made by request to a Memory Manager by passing a memory space. The memory space must have been detected a topology manager from the same or related backend. The following example shows a simple 256-byte allocation made from the first memory space (NUMA Domain) found in the system.

.. code-block:: C++

   // Creating HWloc topology object
  hwloc_topology_t topology;

  // Reserving memory for hwloc
  hwloc_topology_init(&topology);

  // Initializing HWLoC-based topology and memory managers
  HiCR::backend::host::hwloc::L1::TopologyManager tm(&topology);
  HiCR::backend::host::hwloc::L1::MemoryManager mm(&topology);

  // Asking backend to check the available devices
  const auto t = tm.queryTopology();

  // Getting first device found
  auto d = *t.getDevices().begin();

  // Obtaining memory spaces
  auto memSpaces = d->getMemorySpaceList();

  // Getting current process id
  size_t myProcess = rank;

  // Creating local memory slot
  auto firstMemSpace = *memSpaces.begin();
  auto localSlot     = mm.allocateLocalMemorySlot(firstMemSpace, 256);

  // Using memory slot
  ...

  // Freeing memory slot
  mm.freeLocalMemorySlot(localSlot);

Memory Registration
-------------------

If the backend supports it, it is also possible to register a new local memory slot from an existing allocation, assigning it to a given memory space:

.. code-block:: C++

  // External allocation (e.g., from a library)
  size_t size     = 256;
  auto allocation = malloc(size);

  // Registering memory slot
  auto localSlot  = mm.registerLocalMemorySlot(firstMemSpace, allocation, size);

  // Using memory slot
  ...

  // De-registering memory slot
  mm.deregisterLocalMemorySlot(localSlot);

  // Freeing up memory
  free(allocation);
  
.. note::

    As much as possible, it is recommended to create local memory slots by allocation rather than by registration. Allocation gives the backend full control of the associated memory management and may result in a better overall performance.

Local Data Transfer
-------------------

All data transfers, local or remote, in HiCR are made through the communication manager's :code:`memcpy` operation. The following code snippet illustrates how to perform a simple exchange between two local memory slots:

.. code-block:: C++

  // Allocating two one-byte local memory slots
  auto localSlot1 = mm.allocateLocalMemorySlot(firstMemSpace, 1);
  auto localSlot2 = mm.allocateLocalMemorySlot(firstMemSpace, 1);

  // Initializing the first memory slot
  auto value1 = (uint8_t*) localSlot1->getLocalPointer();
  *value1 = 42;

  // Performing the data transfer
  const size_t offset1 = 0;
  const size_t offset2 = 0;
  const size_t sizeToTransfer = 1;
  cm.memcpy(localSlot2, offset2, localSlot1, offset1, sizeToTransfer);

  // Printing value from local memory slot 2
  auto value2 = (uint8_t*) localSlot2->getLocalPointer();
  printf("Transferred value: %u\n", *value2);

It is also possible to specify an offset (in bytes) from the start of the memory slot:

.. code-block:: C++

  const size_t offset1 = 512;
  const size_t offset2 = 1024;
  cm.memcpy(localSlot2, offset2, localSlot1, offset1, sizeToTransfer);

Compute Management
*******************************************

.. note::

    See Related Example: (TBD)

In HiCR, all computation is abstracted behind the Execution Unit class. This class represents functions, kernels, or operations as implemented by each individual backend. For example, a GPU backend may define its execution as a GPU kernel or stream. CPU-based backends may define an execution unit as a simple function or co-routine. The creation of execution units is handled entirely by the Compute Manager class. 

Creating Execution units
--------------------------

The following snippet shows the creation of an execution unit using the :code:`Pthreads` backend:

.. code-block:: C++

  #include <hicr/backends/host/pthreads/L1/computeManager.hpp>

  // Initializing Pthread-based (CPU) compute manager
  HiCR::backend::host::pthreads::L1::ComputeManager computeManager;

  // Defining a function to run
  auto myFunction = [](){ printf("Hello, World!\n"); };

  // Creating execution unit
  auto executionUnit = computeManager.createExecutionUnit(myFunction);

Creating an Execution State
---------------------------

While execution units represent a blueprint of a kernel or function to execution, its actual execution is represented by an Execution State. Execution states represent a single (unique) execution lifetime of an execution unit. When supported, execution states may be suspended, resumed on demand. After the execution state reaches its end, it cannot be re-started. The following snippet shows how to create an execution state from an execution unit:

.. code-block:: C++

  // Creating execution unit
  auto executionState = computeManager.createExecutionState(executionUnit);

Creating Processing Units
---------------------------

Processing Unit are hardware element capable of running an execution state. Compute managers instantiate processing units from an compatible compute resource, as detected by a topology manager. The following snippet shows how to create and initialize a processing unit.

.. code-block:: C++

  #include <hicr/backends/host/pthreads/L1/computeManager.hpp>
  #include <hicr/backends/host/hwloc/L1/topologyManager.hpp>

  // Asking backend to check the available devices
  auto topology = topologyManager.queryTopology();

  // Getting first device found in the topology
  auto device = topology.getDevices()[0];

  // Getting first compute resource found in the device
  auto computeResource = device->getComputeResourceList()[0];

  // Creating processing unit from the compute resource
  auto processingUnit = computeManager.createProcessingUnit(computeResource);

  // Initializing the processing unit (This is necessary step. In this case, it creates the associated Pthread)
  processingUnit->initialize();


Managing an Execution State
-----------------------------

The following snippet shows how to use a processing unit to run an execution state:

.. code-block:: C++

  // Starting the execution of the execution state
  // std::move is necessary here as the execution state is a non-replicable, stateful object
  processingUnit->start(std::move(executionState));

If the backend allows for it, the processing may be suspended (and resumed), even if it is currently running an execution state:

.. code-block:: C++

  // Suspending processing unit
  processingUnit->suspend();

  // Resuming processing unit
  processingUnit->resume();

Finally, the following snippet shows how to synchronously wait for a processing unit to be done running an execution state:

.. code-block:: C++

  // Suspend currently running thread until the processing unit has finished
  processingUnit->await();

RPC Execution
***************************************

.. note::

    See Related Example: (TBD)


HiCR enables instance to launch the execution of Remote Procedure Calls (RPC) on other HiCR instances. For an RPC to execute correctly, three conditions must be satisfied:

* An RPC target must be defined by the receiver. This target is a function mapped to a numerical key and no return value.
* The receiver enters 'listen' mode, which blocks the caller thread until an RPC is received.
* The caller must invoke the RPC target by specifying the target instance and the RPC key.

Declaring RPC Targets
-----------------------

RPCs are entirely managed by the Instance Manager class. The following snippet shows how a receiver instance can declare an RPC target and listen for incoming RPC requests:

..  code-block:: C++

  #define RPC1_TARGET_ID 1
  #define RPC2_TARGET_ID 2
  #define RPC3_TARGET_ID 3

  // Adding RPC targets by id
  instanceManager.addRPCTarget(RPC1_TARGET_ID, []() { printf("Running RPC Target 1\n"); });
  instanceManager.addRPCTarget(RPC2_TARGET_ID, []() { printf("Running RPC Target 2\n"); });
  instanceManager.addRPCTarget(RPC3_TARGET_ID, []() { printf("Running RPC Target 3\n"); });

  // Listening for RPC requests
  instanceManager.listen();

RPC Invokation
-----------------------

RPCs are executed as soon as a request is recevied from the caller instance. The following snippet shows how to send RPC requests to all other instances.

..  code-block:: C++

  // Querying instance list
  auto &instances = instanceManager.getInstances();

  // Getting the pointer to our own (coordinator) instance
  auto myInstance = instanceManager.getCurrentInstance();

  // Printing instance information and invoking a simple RPC if its not ourselves
  for (const auto &instance : instances) if (instance->getId() != myInstance->getId())
  {
     instanceManager.launchRPC(*instance, RPC3_TARGET_ID);
     instanceManager.launchRPC(*instance, RPC2_TARGET_ID);
     instanceManager.launchRPC(*instance, RPC1_TARGET_ID);
  }

Assuming only two instances were running, the result should look as follows:

..  code-block:: bash

  Running RPC Target 3
  Running RPC Target 2
  Running RPC Target 1

.. note::

    HiCR guarantees that RPCs request sent from instance A to instance B will execute in the order in which they were sent. No other ordering is guaranteed. 

RPC Return Values
-----------------------

RPC Return values must be explicitly sent and received via methods to the Instance Manager class inside the RPC function, as follows:

..  code-block:: C++

  // Adding RPC targets by id
  instanceManager.addRPCTarget(RPC1_TARGET_ID, [&]()
  {
    int returnValue = 42;
    printf("Returning value: %d\n", returnValue);
    instanceManager.submitReturnValue(&returnValue, sizeof(returnValue));
  });

The RPC caller can then wait for the reception of the return value by specifying the RPC reciever instance, as follows:

..  code-block:: C++

    instanceManager.launchRPC(*someInstance, RPC1_TARGET_ID);

    // Getting return value
    auto returnValue = *(int *)instanceManager.getReturnValue(*someInstance);

    // Printing return value
    printf("Obtained return value: %d\n", returnValue);

Remote Communication
***************************************

Exchanging Memory Slots
-------------------------

.. note::

    See Related Example: (TBD)

Remote communication is achieved through a backend's communication manager's :code:`memcpy` operation, where at least one of the source / destination arguments is a Remote Memory Slot. Unlike local memory slots, remote memory slots are not directly allocated / registers. Instead, they are exchanged between two or more intervining instances through a collective operation. 

The arguments for the exchange are local memory slots which, after the operation, become visible and accessible by the other instances. The following code snippet illustrates such an exchange:

..  code-block:: C++

  // Declaring a tag to differentiate this particular memory slot exchange from others  
  #define EXCHANGE_TAG 1

  // Creating local memory slot
  auto localSlot = memoryManager.allocateLocalMemorySlot(memorySpace, BUFFER_SIZE);

  // Exchanging our local memory slot to other instances
  communicationManager.exchangeGlobalMemorySlots(EXCHANGE_TAG, {{myInstanceId, localSlot}});

  // Synchronizing so that all instances have finished registering their global memory slots
  communicationManager.fence(EXCHANGE_TAG);

It is also possible to exchange multiple memory slots in one exchange with the same tag:

..  code-block:: C++

  // Creating local memory slots
  auto localSlot1 = memoryManager.allocateLocalMemorySlot(memorySpace, BUFFER_SIZE);
  auto localSlot2 = memoryManager.allocateLocalMemorySlot(memorySpace, BUFFER_SIZE);

  // Exchanging our local memory slot to other instances
  communicationManager.exchangeGlobalMemorySlots(EXCHANGE_TAG, {{myInstanceId * 2 + 0, localSlot1}, {myInstanceId * 2 + 1, localSlot2}};

Or have a particular instance participate in an exchange without contributing a memory slot:

..  code-block:: C++

  // Only receiving remote memory slots references from other instances
  communicationManager.exchangeGlobalMemorySlots(EXCHANGE_TAG, {});

Once the exchange is made, it is possible to extract references to remote memory slots (i.e., those allocated in other HiCR instances) by indicating the desired tag / key pair.

..  code-block:: C++

  // Retreiving the remote (global) memory slot from another instance
  auto remoteSlot = c.getGlobalMemorySlot(EXCHANGE_TAG, remoteInstanceId);

Performing Communication
-------------------------

A memory exchange can be now performed with a remote-to-local (get) or local-to-remote (put) memcpy. After that, calling a fence on the associated tag is required to ensure the operation has completed:

..  code-block:: C++

  // Copying data from a remote instance to our local space (get)
  const size_t localOffset = 0;
  const size_t remoteOffset = 0;
  cm.memcpy(localSlot, localOffset, remoteSlot, remoteOffset, BUFFER_SIZE);

  // Making sure the data has arrived
  cm.fence(EXCHANGE_TAG);

  // Now copying data from our local space to a remote instance (put)
  cm.memcpy(remoteSlot, remoteOffset, localSlot, localOffset, BUFFER_SIZE);

  // Making sure the buffer is ready for reuse
  cm.fence(EXCHANGE_TAG);

Zero-Fence Synchronization
--------------------------

It is possible to query a memory slot for incoming messages without the need synchronizing the intervening instances by querying the number of messages it has received or sent. For this to work, the local memory slots must have been exchanged with the other instances. For example, the operation above can be optimized as follows:

..  code-block:: C++

  // Retreiving my local memory slot as a global memory slot
  auto mySlot = c.getGlobalMemorySlot(EXCHANGE_TAG, myInstanceId);

  // Retreiving the remote (global) memory slot from another instance
  auto remoteSlot = c.getGlobalMemorySlot(EXCHANGE_TAG, remoteInstanceId);

  // Copying data from a remote instance to our local space (get)
  const size_t localOffset = 0;
  const size_t remoteOffset = 0;
  cm.memcpy(mySlot, localOffset, remoteSlot, remoteOffset, BUFFER_SIZE);

  // Busy-waiting until data has arrived (zero-fence)
  while (mySlot.getMessagesRecv() == 0) cm.queryMemorySlotUpdates(mySlot);

  // Now copying data from our local space to a remote instance (put)
  cm.memcpy(remoteSlot, remoteOffset, localSlot, mySlot, BUFFER_SIZE);

  // Busy-waiting until buffer is ready to reuse (zero-fence)
  while (mySlot.getMessagesSent() == 0) cm.queryMemorySlotUpdates(mySlot);

