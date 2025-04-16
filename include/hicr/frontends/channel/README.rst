.. _channels:

***********************
Channels
***********************

Channels are a frontend for data exchange between instances, built on top of the :code:`memcpy` of the Core API (See :ref:`memcpy distributed`). A channel is a global, distributed entity, which consists of at least one producer channel, and one consumer channel in its simplest form.

There are a following different channel versions implemented (each line being a version), based on the size of the elements, the type of channel (single-producer or multiple-producer at the moment), and the policy, which is currently implemented for MPSC:

.. list-table:: Channels
   :widths: 50 50 50
   :header-rows: 1

   * - Element sizes
     - Channel type
     - Policy
   * - Fixed-Size
     - SPSC
     -
   * - Variable-Size
     - SPSC
     -
   * - Fixed-Size
     - MPSC
     - Locking
   * - Variable-Size
     - MPSC
     - Locking
   * - Fixed-Size
     - MPSC
     - Nonlocking
   * - Variable-Size
     - MPSC
     - Nonlocking

The locking policy implies that all producers attempt to lock the same shared channel, and the nonlocking policy implies that all producers nonlockingly access dedicated channels.

.. _channel-instantiation:

Instantiate channels
====================

Channels themselves do not allocate memory, hence the user needs to first allocate following memory slots:

* **Coordination buffers** need to be allocated at both producer and consumer, and they need to be globally advertised
* A **capacity** (in number of tokens), and a **token size** (in bytes) needs to be specified for the channel at both producer and consumer
* A **token buffer** of the specified capacity needs to be created at the consumer side, and needs to be advertised to the producer

:ref:`communication mangement` has more on allocating memory slots and making them globally available.

Producer
----------------------

A producer might declare a channel as follows:

..  code-block:: C++

  // Creating producer channel
  auto producer =
    HiCR::channel::fixedSize::SPSC::Producer(communicationManager, 
                                             tokenBuffer, 
                                             coordinationBuffer, 
                                             consumerCoordinationBuffer, 
                                             sizeof(ELEMENT_TYPE), 
                                             channelCapacity);

Consumer
----------------------

A consumer might declare a channel as follows:

..  code-block:: C++

  // Creating consumer channel
  auto consumer =
    HiCR::channel::fixedSize::SPSC::Consumer(communicationManager, 
                                             globalTokenBufferSlot, 
                                             coordinationBuffer, 
                                             producerCoordinationBuffer, 
                                             sizeof(ELEMENT_TYPE), 
                                             channelCapacity);


Using channels
==============

The channels have very few available operations:

* Querying and updating the current channel size (Consumer/Producer)
* Pushing a token from a memory slot into the channel (Producer)
* Peeking the position of the next available entry in the channel, and popping it from the channel (Consumer)

Query and update channel utilization
-------------------------------------

Both consumer and producer might query the current channel utilization. E.g. a consumer might busy wait on receiving a token as follows:

..  code-block:: C++

  while (consumer.getDepth() < 1) consumer.updateDepth();

Send element
------------

A producer can push tokens into a channel, from example using a local memory slot:

..  code-block:: C++

  // Allocating a send slot to put the values we want to communicate
  ELEMENT_TYPE sendBuffer    = 42;
  auto         sendBufferPtr = &sendBuffer;
  auto         sendSlot      = memoryManager.registerLocalMemorySlot(bufferMemorySpace, sendBufferPtr, sizeof(ELEMENT_TYPE));

  producer.push(sendSlot);

Receive element
---------------

A consumer might inspect an element by getting its position with the peek operation first, and when done, popping them from the channel:

..  code-block:: C++

  // Getting internal pointer of the token buffer slot
  auto tokenBuffer = (ELEMENT_TYPE *)tokenBufferSlot->getPointer();
  printf("Received Value: %u\n", tokenBuffer[consumer.peek()]);
  consumer.pop();
 
.. note::
  For locking channels, such as the locking MPSC, push and pop have a special semantics. Instead of returning void, they return a boolean, which returns true/false depending on the success status of the operation on the limited shared resource. In this case, a busy waiting loop with push/pop is more sensible.

API reference available: `Doxygen <../../../doxygen/html/dir_7e0f30d5b1a3553ca3567294ffe88b4f.html>`_