.. _ascend backend:

***********************
Ascend
***********************

The Ascend (`hiAscend <https://www.hiascend.com/>`_) backend provides the following management functionalities:

* Topology: detect available Ascend devices
* Memory: allocate memory on host (on pinned pages) and on the device
* Computation: enable execution of kernels on the device
* Communication: enable data movements from and to the device, and vice versa. Move data across devices and within the same device  

.. note:: 
    The memory manager supports :code:`MemorySpaces` provided by :ref:`hwloc backend` to trigger host memory allocation. 
    :ref:`hwloc backend` Local Memory Slots can be used with :code:`memcpy`.
