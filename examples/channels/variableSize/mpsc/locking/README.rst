.. _Locking Variable-Size MPSC:

Locking Variable-Size MPSC
==========================

For running this example, both the number of producers and the size of the buffers is configurable per command line. For example:

* :code:`mpirun -n 16 ./mpi 256` launches the examples with 15 producers and a consumer buffer of size 256 tokens, which *is shared among all the producers*.

In the example, each producer will push 3 different-size arrays into the consumer buffer: a 5-element array, a 4-element array, and a 3-element array. The elements are initialized at each producer as:

* `{producerId, 0, producerId, 2 * producerId, 3 * producerId}`
* `{producerId, 4 * producerId, 5 * producerId, 6 * producerId}`
* `{producerId, 7 * producerId, 8 * producerId}`


Push tokens
------------

The semantics for locking channels is different from most other channel versions, as the channel is a shared resource with many producers. The push returns the success status of the operation, and to ensure success, busy waiting might be needed:

.. code-block:: C++

  while (producer.push(sendSlot1) == false);

Receive tokens
-----------------

For each producer, the consumer will first recieve, print, and pop all the tokens, one by one. Again, the channel is a shared resource with many producers, so the pop immediately returns the success status of the operation. To ensure success, busy waiting might be needed:

.. code-block:: C++

    while (consumer.pop() == false);


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
