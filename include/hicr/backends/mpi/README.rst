.. _mpi backend:

***********************
MPI
***********************

The MPI backend provides the most portable backend for remote communication in HiCR,
since MPI itself(`MPI Forum <https://www.mpi-forum.org/>`_) itself provides support for various hardware protocols and networks.
The MPI backend is currently the only choice if you do not have a specialized Infiniband interconnect, where you can also rely on the :ref:`lpf backend` backend specifically designed for Infiniband networks.
The following management functionalities are implemented:

* Instance: Detect available MPI instances
* Memory: expose MPI memory allocation capabilities
* Communication: enable data movements among different instances

API reference available: `Doxygen <../../../doxygen/html/dir_cc8d11758c8576e64833aae972b40fb8.html>`_

.. note:: 
    Examples: :ref:`memcpy distributed`, :ref:`rpc engine`, :ref:`topology distributed`, :ref:`channels`