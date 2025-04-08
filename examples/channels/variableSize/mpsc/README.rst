.. _Variable-Size MPSC Channels:

Channels: Variable-Size MPSC
============================

As outlined in :ref:`Variable-Size Channels`, variable-sized channels can send tokens of varying sizes. This requires an additional coordination buffer, which denotes the message sizes of messages in the channel.

All MPSC have two different policies:

.. toctree::
    :maxdepth: 4

    locking/README
    nonlocking/README
