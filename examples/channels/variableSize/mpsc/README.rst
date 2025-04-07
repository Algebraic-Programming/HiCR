.. _Variable-Size MPSC Channels:

Channels: Variable-Size MPSC
==============================================================


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
