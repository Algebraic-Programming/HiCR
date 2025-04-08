.. _channelsExample:

Channels
========

In this example, we use the :code:`Channel` frontend to exchange tokens between one/multiple producer(s) and a single consumer. This example is implemented with two backends:

* :code:`include/producer.hpp` contains the semantics for the producer(s)
* :code:`include/consumer.hpp` contains the semantics for the consumer
* :code:`source/` contains variants of the main program implemented under different backends

    * :code:`lpf.cpp` corresponds to the :ref:`lpf backend` backend implementation
    * :code:`mpi.cpp` corresponds to the :ref:`mpi backend` backend implementation

Both the producer and consumer functions receive an instance of the :code:`HiCR::L1::MemoryManager`, for the allocation of the token and coordination buffer(s), and; an instance of :code:`HiCR::L1::CommunicationManager`, for the communication of tokens between the HICR instances. 

For details on each example, choose one of the following:

.. toctree::
    :maxdepth: 4

    fixedSize/README
    variableSize/README
