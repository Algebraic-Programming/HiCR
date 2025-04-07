.. _lpf backend:

***********************
LPF
***********************

The LPF communication backend is implementing the in-house version of Infiniband-based communication. The goal of the LPF backend is to provide a viable and performant alternative to the :ref:`mpi backend` backend.
We rely on the `LPF software <https://github.com/Algebraic-Programming/LPF>`_ for the Infiniband support, provided via the IB Verbs API.

Infiniband networks (specifically with ConnectX-* generation adapters) provide hardware features which can be utilized via HiCR, such as:

* polling on a local adapter via :code:`ibv_poll_cq` for the reception of messages. This allows e.g. to avoid remote communication when ensuring certain communication has completed.
* Infiniband provides support for extended atomics, such as atomic Compare&Swap operations on a remote memory address. This allows HiCR to implement a hardware supported global mutex.

The following management functionalities are implemented:

* Memory: expose LPF memory allocation capabilities
* Communication: enable data movements among different instances using Infiniband

.. note:: 
    Examples: :ref:`memcpy distributed`, :ref:`ospubread`, :ref:`topology distributed`, :ref:`channelsFSPSC`, :ref:`channelsFMPSC`, :ref:`channelsVSPSC`, :ref:`channelsVMPSC`