.. _Variable-Size SPSC Channels:

Channels: Variable-Size SPSC
============================

In contrast to :ref:`Fixed-Size SPSC Channels`, variable-sized channels can send tokens of varying sizes. This requires an additional coordination buffer, which denotes the message sizes of messages in the channel. It makes the declaration of a channel more involved, as now two coordination buffers are needed. For the producer, the declaration of the channel is as follows:

.. code-block:: C++

  // Creating producer and consumer channels
  auto producer = HiCR::channel::variableSize::SPSC::Producer(communicationManager,
                                                              sizeInfoBuffer,
                                                              payloadBuffer,
                                                              sizesBuffer,
                                                              coordinationBufferForCounts,
                                                              coordinationBufferForPayloads,
                                                              consumerCoordinationBufferForCounts,
                                                              consumerCoordinationBufferForPayloads,
                                                              PAYLOAD_CAPACITY,
                                                              sizeof(ELEMENT_TYPE),
                                                              channelCapacity);


For the consumer, the declaration of the channel looks as follows:

.. code-block:: C++

  // Creating producer and consumer channels
  auto consumer = HiCR::channel::variableSize::SPSC::Consumer(communicationManager,
                                                              payloadBuffer /*payload buffer */,
                                                              globalSizesBufferSlot,
                                                              coordinationBufferForCounts,
                                                              coordinationBufferForPayloads,
                                                              producerCoordinationBufferForCounts,
                                                              producerCoordinationBufferForPayloads,
                                                              PAYLOAD_CAPACITY,
                                                              channelCapacity);


The data buffer capacity in this example is fixed to 32 bytes and the size buffer (channel capacity) is configurable per command line. For example:

* :code:`mpirun -n 2 ./mpi 3` launches the examples with a channel capacity 3.

In the example, the producer will send 3 arrays of variable sizes to the consumer:

* `{0,1,2,3}`
* `{4,5,6}`
* `{7,8}`

The consumer will first receive, print, and pop a single token each time, printing its full contents, regardless of the size.

Below is the expected result of the application:

.. code-block:: bash

    =====
    PRODUCER sent:0,1,2,3,
    =====
    =====
    CONSUMER:0,1,2,3,
    =====
    =====
    CONSUMER:4,5,6,
    =====
    =====
    PRODUCER sent:4,5,6,
    =====
    =====
    CONSUMER:7,8,
    =====
    =====
    PRODUCER sent:7,8,
    =====

