.. _Locking Fixed-Size MPSC:

Locking Fixed-Size MPSC
=======================

For running this example, both the number of producers and the size of the buffers is configurable per command line. For example:

* :code:`mpirun -n 16 ./mpi 3` launches the examples with 15 producers and a consumer buffer of size 3.

Push tokens
------------

In the example, each producer will push numeric tokens into the consumer buffer. The semantics here is different from most other channel versions, as the channel is a shared resource with many producers. The push returns the success status of the operation, and to ensure success, busy waiting might be needed:

.. code-block:: C++

  while (producer.push(sendSlot1) == false);

Receive tokens
------------

For each producer, the consumer will first recieve, print, and pop all the tokens, one by one. Again, the channel is a shared resource with many producers, so the pop immediately returns the success status of the operation. To ensure success, busy waiting might be needed:

.. code-block:: C++

    while (consumer.pop() == false);

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

