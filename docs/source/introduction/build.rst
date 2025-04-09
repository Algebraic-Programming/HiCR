.. _building:

************************
Building & Installation
************************

The HiCR Core API is a self-sufficient include-only library that does not require compilation. Its backends, however, may require the presence of dependencies (libraries or drivers) in the system to work and link against. Its frontends may also contain source code that requires compilation. We use the `Meson <https://mesonbuild.com>`_ build system to handle all of these aspects and provide a seamless compilation of HiCR-based applications. 

.. _downloading:

Getting HiCR
***********************

HiCR is released in a continuous basis. The latest release is always the latest :code:`master` branch commit in its `git repository <https://github.com/Algebraic-Programming/HiCR>`_. To obtain the latest release, simply run:

..  code-block:: bash

  git clone https://github.com/Algebraic-Programming/HiCR

.. _configure:

Software Requirements
***********************

Before installing HiCR, make sure the following libraries and tools are installed:

* C++ compiler with suppport for C++20 (e.g., :code:`g++ >= 11.0`)
* :code:`python (version >= 3.9)`
* :code:`meson (version > 1.0.0)`
* :code:`ninja (version > 1.0.0)`
* :code:`gtest`

The following libraries and tools are only necessary for certain HiCR backends:

* :ref:`ascend backend` :code:`ascend-toolkit 7.0.RC1.alpha003`

* :ref:`boost backend` :code:`libboost-context-dev (version >= 1.71)`

* :ref:`hwloc backend` :code:`hwloc (version >= 2.1.0)`

* :ref:`lpf backend` :code:`LPF (version 'noc_extension')`

* :ref:`mpi backend` :code:`openmpi (version >= 5.0.2)`

* :ref:`nosv backend` :code:`nos-v (version >= 3.1.0)`

* :ref:`opencl backend` :code:`intel-opencl-icd (version >= 3.0)`

* :ref:`pthreads backend` :code:`Pthreads (version any)`

Configuration
***********************

HiCR uses `meson` as build and installation system. To build HiCR, you can run the following command:

..  code-block:: bash

  # [Example] Configuring HiCR's meson project, with its default configuration, in the folder "build"
  meson setup build 

  # [Example] Configuring HiCR's meson project, with all its backends, and frontends. The build folder is "build"
  meson setup build -DbuildExamples=true -DbuildTests=true -Dbackends=hwloc,boost,pthreads,mpi,lpf,ascend,nosv,opencl -Dfrontends=channel,RPCEngine,tasking,objectStore 

  # Compiling 
  meson compile -C build

  # or
  cd build
  ninja


.. _buildTests:

Building Tests and Examples
****************************

To compile HiCR's tests and examples, add the corresponding flags in the configuration command

..  code-block:: bash

  # Configuring HiCR's meson project, along with its examples and tests
  meson setup build -DbuildExamples=true -DbuildTests=true

To execute them:

.. code-block:: bash
  
  meson test -C build

.. _installation:

Installation
***********************

By default, HiCR will install in the system's default folder, but this can be configured:

..  code-block:: bash

  # Configuring HiCR's meson project with a non-default install folder
  meson setup build -Dprefix=$HOME/.local

  # Installing
  meson install -C build

.. _running:

Running
***********************

To run a HiCR-based application (or one of the included examples), simply run it as usual:

..  code-block:: bash

  # Running example (from within the build folder)
  examples/topology/hwloc

If the application uses a backend that requires a specific launcher (e.g., MPI), you should use it:

..  code-block:: bash

  # Running MPI-based example
  mpirun -n 2 examples/topologyRPC/mpi
   

