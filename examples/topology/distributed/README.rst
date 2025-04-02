Distributed Topology Example
=============================

This example uses RPC to enable a coordinator instance to get the topology of all the other active instances. The :ref:`rpcengine`


This example showcases how the abstract HiCR Core API can be used to discover compute and memory devices in the system. The code is structured as follows:

* :code:`source/` contains the different variants of this example corresponding to different backends

    * :code:`mpi.cpp` corresponds to the :ref:`mpi` backend implementation. It uses hwloc to discover local resources and then communicates them via MPI 

The discover is initiated by the root instance with an RPC calls that triggers topology discovery in each HiCR Instance. Topologies are sent back to the root instance, merged together, and finally displayed.
More information on the topology discovery can be found in the :code:`local` version of this example.