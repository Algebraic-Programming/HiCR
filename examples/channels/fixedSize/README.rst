.. _Fixed-Size Channels:

Fixed-size Channels
-------------------

For fixed sized channels, the consumer allocates one internal buffer:

* **Token Data Buffer**. This is where the actual token data is stored. This buffer is configured with a maximum size that cannot be exceeded. The producer will fail to push if adding a new token of a given size will exceed the current capacity of the buffer. 

.. toctree::
    :maxdepth: 4

    mpsc/README
    spsc/README
