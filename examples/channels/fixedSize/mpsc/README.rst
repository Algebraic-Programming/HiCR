Channels: Fixed MPSC
==============================================================

In this example, we use the :code:`Channel` frontend to exchange fixed-sized tokens between multiple producers and a single consumer. This example is implemented in two variants:

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

In the example, each producer will send attempt to push three numeric tokens, :code:`42`, :code:`43` and :code:`44`, into the consumer buffer. For each producer, the consumer will first recieve, print, and pop all three tokens, one by one.

Below is a possible expected result of the application for two producers and a buffer size :code:`>= 2`:

.. code-block:: bash

    [Producer 001] Sent Value: 42
    [Producer 004] Sent Value: 42
    [Producer 002] Sent Value: 42
        [Consumer] Recv Value: 42  (1/12) Pos: 0
        [Consumer] Recv Value: 42  (2/12) Pos: 1
        [Consumer] Recv Value: 42  (3/12) Pos: 2
    [Producer 003] Sent Value: 42
    [Producer 003] Sent Value: 43
    [Producer 003] Sent Value: 44
    [Producer 002] Sent Value: 43
        [Consumer] Recv Value: 42  (4/12) Pos: 0
        [Consumer] Recv Value: 43  (5/12) Pos: 1
        [Consumer] Recv Value: 43  (6/12) Pos: 2
    [Producer 001] Sent Value: 43
    [Producer 002] Sent Value: 44
        [Consumer] Recv Value: 44  (7/12) Pos: 0
    [Producer 001] Sent Value: 44
        [Consumer] Recv Value: 43  (8/12) Pos: 1
    [Producer 004] Sent Value: 43
        [Consumer] Recv Value: 44  (9/12) Pos: 2
        [Consumer] Recv Value: 44  (10/12) Pos: 0
        [Consumer] Recv Value: 43  (11/12) Pos: 1
    [Producer 004] Sent Value: 44
        [Consumer] Recv Value: 44  (12/12) Pos: 2

