Memcpy (Local)
=======================

In this example, we test HiCR's :code:`memcpy` operation to run a *Telephone Game* test, where a contiguous memory array is copied along all the memory spaces found in the local HiCR instance, as detected by a :code:`HiCR::L1::TopologyManager`, and then back to the initial space (see :numref:`telephoneGameAPI`). The example then tests the initial and resulting array contain the same bytes. 

.. _telephoneGameAPI:
.. figure:: telephoneGame.png
   :alt: Telephone Game Example
   :align: center
   
   Telephone Game Example

The code is structured as follows:

* :code:`include/telephoneGame.hpp` contains the application's backend-independent semantics
* :code:`source/` contains variants of the main program implemented under different backends

    * :code:`pthreads.cpp` corresponds to the :ref:`pthreads` + :ref:`hwloc` backend implementation. This variant moves the initial allocation across all of the system's RAM NUMA domains.
    * :code:`ascend.cpp` corresponds to the :ref:`ascend` backend implementation. This variant moves the initial allocation across all Ascend GPU devices found.
    * :code:`opencl.cpp` corresponds to the :ref:`opencl` backend implementation. This variant moves the initial allocation across all devices found by the OpenCL platform.

Both the producer and consumer functions receive a set of :code:`HiCR::L0::MemorySpace`, each of which will take a turn in the telephone game. They also receive an instance of the :code:`HiCR::L1::MemoryManager`, for the allocation of local memory slots across all memory spaces provided, and; an instance of :code:`HiCR::L1::CommunicationManager`, to communicate the data the HICR memory spaces. 

The expected result of running this example is:

.. code-block:: bash

    Input: Hello, HiCR user!
    Output: Hello, HiCR user!

