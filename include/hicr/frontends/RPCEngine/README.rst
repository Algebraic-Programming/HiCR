.. _rpcengine:

***********************
RPC Engine
***********************

RPC Engine exposes building blocks to register and invoke remote procedure calls.

HiCR enables instance to launch the execution of Remote Procedure Calls (RPC) on other HiCR instances. For an RPC to execute correctly, three conditions must be satisfied:

* An RPC target must be defined by the receiver. This target is a function mapped to a numerical key and no return value.
* The receiver enters 'listen' mode, which blocks the caller thread until an RPC is received.
* The caller must invoke the RPC target by specifying the target instance and the RPC key.

Declaring RPC Targets
-----------------------

RPCs are entirely managed by the instance manager class. The following snippet shows how a receiver instance can declare an RPC target:

..  code-block:: C++

  #define RPC1_TARGET_ID 1
  #define RPC2_TARGET_ID 2
  #define RPC3_TARGET_ID 3

  // Adding RPC targets by id
  instanceManager.addRPCTarget(RPC1_TARGET_ID, []() { printf("Running RPC Target 1\n"); });
  instanceManager.addRPCTarget(RPC2_TARGET_ID, []() { printf("Running RPC Target 2\n"); });
  instanceManager.addRPCTarget(RPC3_TARGET_ID, []() { printf("Running RPC Target 3\n"); });

RPC Listening
------------------------

After RPCs have been registered, the instance listens for incoming messages:

.. code-block:: c++

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

RPC Return values must be explicitly sent and received via methods to the instance manager class inside the RPC function, as follows:

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
   
API reference available: `Doxygen <../../../doxygen/html/dir_82fb27424b1984dde9c93a14a78e097d.html>`_