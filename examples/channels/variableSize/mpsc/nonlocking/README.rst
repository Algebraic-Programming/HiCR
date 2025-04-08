.. _Nonlocking Variable-Size MPSC:

Nonlocking Variable-Size MPSC
=============================

Nonlocking variable-size channels require large channel capacities, as the consumer then creates a dedicated channel with a fixed capacity *per producer* to avoid locking. Other than that, they are essentially a collection of :ref:`Variable-Size SPSC Channels` and follow the same semantics.

For running this example, both the number of producers and the size of the buffers is configurable per command line. For example:

* :code:`mpirun -n 16 ./mpi 3` launches the examples with 15 producers and a consumer buffer of size 3. In effect, this creates a channel with capacity 45 ( = 15 producers * 3 tokens per channel).

Each producer sends 4 different-sized arrays:

* `{42,43,44,45}`
* `{42,43,44}`
* `{42,43}`
* `{42}`

A possible excerpt from the output could be as follows; the consumer prints in detail the element popped, as well as which position of this 45-token channel we pop from, which dedicated producer channel it is from, and which position from this producer channel (1 to 15) it is from.

.. code-block:: bash

  PRODUCER 3 sent: reading 8 bytes 42,43,
  =====
  =====
  PRODUCER 3 sent: reading 4 bytes 42,
  =====
  =====
  PRODUCER 0 sent: reading 20 bytes 42,43,44,45,46,
  =====
  =====
  PRODUCER 0 sent: reading 16 bytes 42,43,44,45,
  =====
  =====
  PRODUCER 0 sent: reading 12 bytes 42,43,44,
  =====
  =====
  PRODUCER 0 sent: reading 8 bytes 42,43,
  =====
  =====
  PRODUCER 0 sent: reading 4 bytes 42,
  =====
  =====
  CONSUMER @ channel 1  reading 16 bytes 42,43,44,45,
  =====
  =====
  CONSUMER @ channel 2  reading 16 bytes 42,43,44,45,
  =====
  =====
  CONSUMER @ channel 3  reading 20 bytes 42,43,44,45,46,
  =====
  =====
  CONSUMER @ channel 4  reading 16 bytes 42,43,44,45,
  =====
  =====
  CONSUMER @ channel 5  reading 16 bytes 42,43,44,45,
  =====
  =====
  CONSUMER @ channel 0  reading 20 bytes 42,43,44,45,46,
  =====
  =====
  CONSUMER @ channel 1  reading 12 bytes 42,43,44,
  =====
  =====
  CONSUMER @ channel 2  reading 12 bytes 42,43,44,
  =====
  =====
  CONSUMER @ channel 2  reading 8 bytes 42,43,
  =====
