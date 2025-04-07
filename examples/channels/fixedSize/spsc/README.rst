.. _Fixed-Size SPSC Channels:

Channels: Fixed-Size SPSC
==============================================================


The size of the buffers is configurable per command line. For example:

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
    
