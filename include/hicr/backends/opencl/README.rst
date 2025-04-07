.. _opencl backend:

***********************
OpenCL
***********************

The OpenCL(`OpenCL <https://www.khronos.org/opencl/>`_) backend provides the following management functionalities:

* Topology: detect available OpenCL devices
* Memory: allocate memory on host (on pinned pages) and on the device
* Computation: enable execution of kernels on the device
* Communication: enable data movements from and to the device, and vice versa. Move data across devices and within the same device  

.. note:: 
    Examples: :ref:`memcpy local`, :ref:`kernel execution`, :ref:`topology local` 