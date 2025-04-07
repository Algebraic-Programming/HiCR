.. _channelsFSPSC:

Channels: Fixed SPSC
==============================================================

In this example, we use the :code:`Channel` frontend to exchange fixed-sized tokens between a single producer and a single consumer. The code is structured as follows:

* :code:`include/producer.hpp` contains the semantics for the producer
* :code:`include/consumer.hpp` contains the semantics for the consumer
* :code:`source/` contains variants of the main program implemented under different backends

    * :code:`lpf.cpp` corresponds to the :ref:`lpf backend` backend implementation
    * :code:`mpi.cpp` corresponds to the :ref:`mpi backend` backend implementation

Both the producer and consumer functions receive an instance of the :code:`HiCR::MemoryManager`, for the allocation of the token and coordination buffer(s), and; an instance of :code:`HiCR::CommunicationManager`, for the communication of tokens between the HICR instances. The size of the buffers is configurable per command line. For example:

* :code:`mpirun -n 2 ./mpi 3` launches the examples with a consumer buffer of size 3.

In the example, the producer will send attempt to push three numeric tokens, :code:`42`, :code:`43` and :code:`44`, into the consumer buffer. The consumer will first recieve, print, and pop a single token, and then it will wait until two more messages have arrived to print and pop. 

Below is the expected result of the application for a buffer size :code:`>= 2`:

.. code-block:: bash

    Received Value: 42
    Received Value: 43
    Received Value: 44
    Sent Value:     42
    Sent Value:     43
    Sent Value:     44

However, when running the application if a buffer size :code:`= 1`, the consumer will lock waiting for two successive messages, when the producer can only send one at a time:

.. code-block:: bash

    Sent Value:     42
    Received Value: 42
    Sent Value:     43
    
