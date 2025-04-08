.. _Variable-Size Channels:

Variable-Size Channels
----------------------

Variable-sized channels can send tokens of varying sizes. This requires an additional coordination buffer, which denotes the message sizes of messages in the channel.
The consumer allocates two internal buffers:

* **Token Data Buffer**. This is where the actual token data is stored. This buffer is configured with a maximum size that cannot be exceeded. The producer will fail to push if adding a new token of a given size will exceed the current capacity of the buffer. 
* **Token Size Buffer**. This buffer holds the size (and starting position) of the currently received tokens. The producer will fail to push if adding a new token of this buffer is full, even if the data buffer has enough space lefta given size will exceed the current capacity of the buffer. 

.. toctree::
    :maxdepth: 4

    mpsc/README
    spsc/README
