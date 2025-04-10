.. _ascend backend:

***********************
Ascend
***********************

The Ascend (`hiAscend <https://www.hiascend.com/>`_) backend provides the following management functionalities:

* Topology: detect available Ascend devices
* Memory: allocate memory on host (on pinned pages) and on the device
* Computation: enable execution of kernels on the device
* Communication: enable data movements from and to the device, and vice versa. Move data across devices and within the same device  

API reference available: `Doxygen <../../../doxygen/html/dir_42b7d869cd2bc092c1bc66b28875e517.html>`_

Checkout which version is required: :ref:`software requirements`

.. note:: 
    Examples: :ref:`memcpy local`, :ref:`kernel execution`, :ref:`topology local` 