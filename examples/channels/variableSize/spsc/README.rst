.. _Variable-Size SPSC Channels:

Channels: Variable-Size SPSC
============================

The data buffer capacity in this example is fixed to 32 bytes and the size buffer (channel capacity) is configurable per command line. For example:

* :code:`mpirun -n 2 ./mpi 3` launches the examples with a channel capacity 3.

In the example, the producer will send 3 arrays of variable sizes to the consumer:

* `{0,1,2,3}`
* `{4,5,6}`
* `{7,8}`

The consumer will first receive, print, and pop a single token each time, printing its full contents, regardless of the size.

Details on each single operation are the same as :ref:`Fixed-Size SPSC Channels`

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

