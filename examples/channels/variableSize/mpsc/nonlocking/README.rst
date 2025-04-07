.. _Nonlocking Variable-Size MPSC:

Nonlocking Variable-Size MPSC
=============================

Nonlocking variable-size channels require large channel capacities, as the consumer then creates a dedicated channel with a fixed capacity *per producer* to avoid locking.

For running this example, both the number of producers and the size of the buffers is configurable per command line. For example:

* :code:`mpirun -n 16 ./mpi 3` launches the examples with 15 producers and a consumer buffer of size 3. In effect, this creates a channel with capacity 45 ( = 15 producers * 3 tokens per channel).

A possible excerpt from the output could be as follows; the consumer prints in detail the element popped, as well as which position of this 45-token channel we pop from, which dedicated producer channel it is from, and which position from this producer channel (1 to 15) it is from.

.. code-block: bash

  [Producer 005] Sent Value: 43
  [Producer 005] Sent Value: 44
  [Producer 013] Sent Value: 42
  [Producer 013] Sent Value: 43
  [Producer 013] Sent Value: 44
  [Producer 009] Sent Value: 42
  [Producer 009] Sent Value: 43
  [Producer 009] Sent Value: 44
  [Producer 007] Sent Value: 42
  [Producer 007] Sent Value: 43
  [Producer 007] Sent Value: 44
      [Consumer] Recv Value: 42  (1/45) Pos: 0 @ SPSC Channel 3
      [Consumer] Recv Value: 43  (2/45) Pos: 1 @ SPSC Channel 3
      [Consumer] Recv Value: 44  (3/45) Pos: 2 @ SPSC Channel 3
      [Consumer] Recv Value: 42  (4/45) Pos: 0 @ SPSC Channel 5
      [Consumer] Recv Value: 43  (5/45) Pos: 1 @ SPSC Channel 5
      [Consumer] Recv Value: 44  (6/45) Pos: 2 @ SPSC Channel 5
      [Consumer] Recv Value: 42  (7/45) Pos: 0 @ SPSC Channel 7
      [Consumer] Recv Value: 43  (8/45) Pos: 1 @ SPSC Channel 7
      [Consumer] Recv Value: 44  (9/45) Pos: 2 @ SPSC Channel 7
      [Consumer] Recv Value: 42  (10/45) Pos: 0 @ SPSC Channel 9
      [Consumer] Recv Value: 43  (11/45) Pos: 1 @ SPSC Channel 9
      [Consumer] Recv Value: 44  (12/45) Pos: 2 @ SPSC Channel 9
      [Consumer] Recv Value: 42  (13/45) Pos: 0 @ SPSC Channel 11
