.. _channels:

***********************
Channels
***********************

Channels are a frontend for data exchange between instances, built on top of the memcpy of the Core API (See :ref:`Memcpy Dist`). A channel really is a global, distributed entity, which consists of at least one producer channel and one consumer channel in its simplest form.

Declaring channels
===================

Channels themselves do not allocate memory, hence the user needs to first allocate following memory slots:
* Small coordination buffers need to be allocated at both producer and consumer, and they need to be globally advertised
* A token buffer of a specified capacity needs to be created at the consumer side, and needs to be advertised to the producer

A producer might declare a channel as follows:
..  code-block:: C++
  // Creating producer channel
  auto producer =
    HiCR::channel::fixedSize::SPSC::Producer(communicationManager, tokenBuffer, coordinationBuffer, consumerCoordinationBuffer, sizeof(ELEMENT_TYPE), channelCapacity);

A consumer might declare a channel as follows:
..  code-block:: C++
  // Creating consumer channel
  auto consumer =
    HiCR::channel::fixedSize::SPSC::Consumer(communicationManager, globalTokenBufferSlot, coordinationBuffer, producerCoordinationBuffer, sizeof(ELEMENT_TYPE), channelCapacity);


Using channels
==============

The channels have very few available operations. Both consumer and producer might query the current channel utilization. E.g. a consumer might busy wait on receiving a token as follows:
..  code-block:: C++
  while (consumer.getDepth() < 1) consumer.updateDepth();

A producer can push tokens into a channel, from example using a local memory slot:
..  code-block:: C++
  // Allocating a send slot to put the values we want to communicate
  ELEMENT_TYPE sendBuffer    = 42;
  auto         sendBufferPtr = &sendBuffer;
  auto         sendSlot      = memoryManager.registerLocalMemorySlot(bufferMemorySpace, sendBufferPtr, sizeof(ELEMENT_TYPE));

  producer.push(sendSlot);

A consumer might inspect an element by getting its position with the peek operation first, and when done, popping them from the channel:
..  code-block:: C++
  // Getting internal pointer of the token buffer slot
  auto tokenBuffer = (ELEMENT_TYPE *)tokenBufferSlot->getPointer();
  printf("Received Value: %u\n", tokenBuffer[consumer.peek()]);
  consumer.pop();
 
.. note::
  For locking channels, such as the locking MPSC, push and pop have a special semantics. Instead of returning void, they return a boolean, which returns true/false depending on the success status of the operation on the limited shared resource. In this case, a busy waiting loop with push/pop is more sensible.
