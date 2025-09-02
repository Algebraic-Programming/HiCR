.. _topology local:

Topology: Local
===============

This example showcases how the abstract HiCR Core API can be used to discover compute and memory devices in the system. The code is structured as follows:

* :code:`source/` contains the different variants of this example corresponding to different backends

    * :code:`acl.cpp` corresponds to the :ref:`acl backend` backend implementation
    * :code:`hwloc.cpp` corresponds to the :ref:`hwloc backend` (CPU) backend implementation

Topology detection
------------------

First, we use the topology manager to query the local HiCR instance's topology

.. code-block:: C++

   auto topology = topologyManager.queryTopology();

Compute resources and memory spaces retrieval
---------------------------------------------

Then, we iterate over all its devices, printing the compute resources and memory spaces we find along with their types. For the latter, also their capacity.

.. code-block:: C++

  for (const auto &d : topology.getDevices())
   {
     auto c = d->getComputeResourceList(); // Used to print compute resources
     auto m = d->getMemorySpaceList(); // Used to print memory spaces
   }
    

The result of running the hwloc example might be, for example:

.. code-block:: bash

  + 'NUMA Domain'
    Compute Resources: 44 Processing Unit(s)
    Memory Space:     'RAM', 94.483532 Gb
  + 'NUMA Domain'
    Compute Resources: 44 Processing Unit(s)
    Memory Space:     'RAM', 93.024166 Gb

Whereas, for the acl example, it would look as follows:

.. code-block:: bash
    
    + 'Huawei Device'
      Compute Resources: 1 Huawei Processor(s)
      Memory Space:     'Huawei Device RAM', 32.000000 Gb
    + 'Huawei Device'
      Compute Resources: 1 Huawei Processor(s)
      Memory Space:     'Huawei Device RAM', 32.000000 Gb

and for OpenCL, for example:

.. code-block:: bash
    
  + 'OpenCL GPU'
    Compute Resources: 1 OpenCL GPU Processing Unit(s)
    Memory Space:     'OpenCL Device RAM', 49.908493 Gb
