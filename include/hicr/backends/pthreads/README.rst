.. _pthreads backend:

***********************
Pthreads
***********************

The Pthreads(`Pthreads <https://man7.org/linux/man-pages/man7/pthreads.7.html>`_) backend provides means to execute functions using Pthreads. The following management functionalities are implemented:

* Computation: enable executing functions in a Pthread
* Communication: move data via standard memcpy

API reference available: `Doxygen <../../../doxygen/html/dir_cb2e0100c474338ea507add51adcb71e.html>`_

.. note:: 
    Examples: :ref:`memcpy local`, :ref:`kernel execution`, :ref:`topology distributed`, :ref:`taskingExample`