.. _pingPongExample:

Ping-Pong Example
=================

In this example, we use the :code:`Channel` frontend to exchange tokens between exactly two processes, both collectively creating two fixed-size SPSC Channels (see :ref:`Fixed-Size SPSC Channels`). The ping channel of a consumer pings a message to a consumer, who echoes the message back to the producer via a pong channel. 

Three arguments are required for the example:

* The channel capacity
* The number of repetitions of a ping-pong between the producer and consumer
* The size (in bytes) of a single token being ping-ponged

For example:

* :code:`mpirun -n 2 ./mpi 5 10 256` launches the examples to create 2 channels of capacity 5 tokens each, each token being 256 bytes large. The ping-pong is repeated 10 times.

The output shows the benchmarked execution time of the ping-pong itself at the producer:

 .. code-block:: bash

 Time: 0.012845 seconds

This example is implemented with two backends:

* :code:`include/producer.hpp` contains the semantics for the producer(s)
* :code:`include/consumer.hpp` contains the semantics for the consumer
* :code:`source/` contains variants of the main program implemented under different backends

    * :code:`lpf.cpp` corresponds to the :ref:`lpf backend` backend implementation
    * :code:`mpi.cpp` corresponds to the :ref:`mpi backend` backend implementation

