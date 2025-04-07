.. _channelsVMPSC:

Channels: Variable MPSC
==============================================================

In this example, we use the :code:`Channel` frontend to exchange variable-sized tokens between multiple producers and a single consumer. This example is implemented in two variants:

* :code:`locking/` Contains a variant where the consumer keeps a single buffer and producers use a mutual exclusion mechanism to access it. This adds computational complexity to the push operation.
* :code:`nonlocking` Contains a variant where the consumer keeps one buffer per producer, allowing them to operate concurrently. This adds computational complexity to the peek and pop operations.

In both variants, the code is structured as follows:

* :code:`include/producer.hpp` contains the semantics for the producer(s)
* :code:`include/consumer.hpp` contains the semantics for the consumer
* :code:`source/` contains variants of the main program implemented under different backends

    * :code:`lpf.cpp` corresponds to the :ref:`lpf backend` backend implementation
    * :code:`mpi.cpp` corresponds to the :ref:`mpi backend` backend implementation

Both the producer and consumer functions receive an instance of the :code:`HiCR::L1::MemoryManager`, for the allocation of the token and coordination buffer(s), and; an instance of :code:`HiCR::L1::CommunicationManager`, for the communication of tokens between the HICR instances. 

For running this example, both the number of producers and the size of the buffers is configurable per command line. For example:

* :code:`mpirun -n 16 ./mpi 3` launches the examples with 15 producers and a consumer buffer of size 3.

In the example, each producer will send attempt to push numeric tokens, into the consumer buffer. For each producer, the consumer will first recieve, print, and pop all the tokens, one by one.

Below is a possible expected result of the application for two producers and a buffer size :code:`>= 2`:

.. code-block:: bash

    =====
    PRODUCER 7 sent: reading 20 bytes 7,0,7,14,21,
    =====
    =====
    CONSUMER receives from PRODUCER 7: reading 20 bytes 7,0,7,14,21,
    =====
    =====
    PRODUCER 3 sent: reading 20 bytes 3,0,3,6,9,
    =====
    =====
    CONSUMER receives from PRODUCER 3: reading 20 bytes 3,0,3,6,9,
    =====
    =====
    PRODUCER 7 sent: reading 16 bytes 7,28,35,42,
    =====
    =====
    CONSUMER receives from PRODUCER 7: reading 16 bytes 7,28,35,42,
    =====