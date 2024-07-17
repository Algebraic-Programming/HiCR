Channels: Variable SPSC
==============================================================

In this example, we use the :code:`Channel` frontend to exchange variable-sized tokens between a single producer and a single consumer. The code is structured as follows:

* :code:`include/producer.hpp` contains the semantics for the producer
* :code:`include/consumer.hpp` contains the semantics for the consumer
* :code:`source/` contains variants of the main program implemented under different backends

    * :code:`lpf.cpp` corresponds to the :ref:`lpf` backend implementation
    * :code:`mpi.cpp` corresponds to the :ref:`mpi` backend implementation

Both the producer and consumer functions receive an instance of the :code:`HiCR::L1::MemoryManager`, for the allocation of the token and coordination buffer(s), and; an instance of :code:`HiCR::L1::CommunicationManager`, for the communication of tokens between the HICR instances. 

For variable sized channels, the consumer allocates two internal buffers:

* **Token Data Buffer**. This is where the actual token data is stored. This buffer is configured with a maximum size that cannot be exceeded. The producer will fail to push if adding a new token of a given size will exceed the current capacity of the buffer. 
* **Token Size Buffer**. This buffer holds the size (and starting position) of the currently received tokens. The producer will fail to push if adding a new token of this buffer is full, even if the data buffer has enough space lefta given size will exceed the current capacity of the buffer. 

The data buffer capacity in this example is fixed to 32 and the size buffer (channel capacity) is configurable per command line. For example:

* :code:`mpirun -n 2 ./mpi 3` launches the examples with a channel capacity 3.

In the example, the producer will send multiple messages of variable size into the consumer buffer. The consumer will first recieve, print, and pop a single token each time, printing its full contents, regardless of the size.

Below is the expected result of the application:

.. code-block:: bash

    PRODUCER sent:0,1,2,3
    PRODUCER sent:4,5,6
    PRODUCER sent:7,8
    CONSUMER:0,1,2,3
    CONSUMER:4,5,6
    CONSUMER:7,8

