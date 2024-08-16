.. _building:

************************
Building & Installation
************************

The HiCR Core API is a self-sufficient include-only library that does not require compilation. Its backends, however, may require the presence of dependencies (libraries or drivers) in the system to work and link against. Its frontends may also contain source code that requires compilation. We use the `Meson <https://mesonbuild.com>`_ build system to handle all of these aspects and provide a seamless compilation of HiCR-based applications. 

.. _downloading:

Getting HiCR
***********************

HiCR is released in a continuous basis. The latest release is always the latest :code:`master` branch commit in its `git repository <https://gitlab.huaweirc.ch/zrc-von-neumann-lab/runtime-system-innovations/hicr>`_. To obtain the latest release, simply run:

..  code-block:: bash

  git clone https://gitlab.huaweirc.ch/zrc-von-neumann-lab/runtime-system-innovations/hicr.git

.. _configure:

Configuration
***********************

HiCR uses `meson` as build and installation system. To build HiCR, you can run the following command:

..  code-block:: bash

  # Creating and entering a build folder.
  mkdir build 
  cd build

  # [Example] Configuring HiCR's meson project, with its basic configuration
  meson .. 

  # [Example] Configuring HiCR's meson project, with all its backends, and frontends.
  meson .. -DbuildExamples=true -DbuildTests=true -Dbackends=host/hwloc,host/pthreads,mpi,lpf,ascend -Dfrontends=channel,deployer,machineModel,tasking

  # Compiling 
  ninja


.. _buildTests:

Building Tests and Examples
****************************

To compile HiCR's tests and examples, add the corresponding flags in the configuration command

..  code-block:: bash

  # Configuring HiCR's meson project, along with its examples and tests
  meson .. -DbuildExamples=true -DbuildTests=true


.. _installation:

Installation
***********************

By default, HiCR will install in the system's default folder, but this can be configured:

..  code-block:: bash

  # Configuring HiCR's meson project with a non-default install folder
  meson .. -Dprefix=$HOME/.local

  # Installing
  ninja install

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
   

