.. _contributing:

***********************
Contributing
***********************

Thank you for investing your time in contributing to our project!

In this guide you will get an overview of the contribution workflow from opening an issue, creating and merge a PR, develop your own HiCR backend.

Contribute to HiCR
*******************

To get an overview of the project, read the `README <https://github.com/Algebraic-Programming/HiCR/blob/master/README.md>`_, and the `user manual <https://aaaa>`_.

Create a new Issue
====================

If you spot a problem, search if an `issue <https://github.com/Algebraic-Programming/HiCR/issues>`_ already exists. If a related issue doesn't exist, you can open a new one using `this template <https://github.com/Algebraic-Programming/HiCR/blob/master/.github/ISSUE_TEMPLATE/>`__.

Create a new PR
================

Create a pull request using `this template <https://github.com/Algebraic-Programming/HiCR/blob/master/.github/PULL_REQUEST_TEMPLATE.md>`__.
Don't forget to link PR to the relevant issue(s), if you are solving some.

During the review process, we may ask for changes to be made before a PR can be merged.
You need at least one approval from one of the mainteiners to get the PR merged.

.. _backendDevelopment:

Backend Development
***********************

We appreciate every effort to expand the HiCR ecosystem to new technologies!
Here you can find a quickstart on implementing a HiCR backend. If you do so, let us know so we can update our `README <https://github.com/Algebraic-Programming/HiCR/blob/master/README.md>`_ with new backends!

Build setup
=============

The standard setup for creating a new backend is to create a new repository, and import HiCR as a dependency using the `Meson <https://mesonbuild.com/>`_ build system.
You have two options to do it:

* Add HiCR as a `git submodule <https://git-scm.com/book/en/v2/Git-Tools-Submodules>`_ to the repository and import HiCR as a `subproject <https://mesonbuild.com/Subprojects.html>`_ :

.. code-block:: bash

  # In a shell
  cd myRepository
  git submodule add https://github.com/Algebraic-Programming/HiCR

  # In meson.build file of myRepository
  HiCRBackends = [...] # put here any additional backend you want to enable
  HiCRProject = subproject('HiCR', required: true, default_options: { 'backends': HiCRBackends })
  
  # Include this variable in the target dependencies to enable HiCR in your application
  HiCRBuildDep = HiCRProject.get_variable('hicrBuildDep')

* If HiCR is installed in the system (see :ref:`installation`), declare HiCR as a `dependency <https://mesonbuild.com/Dependencies.html>`_ of your project:

.. code-block:: bash

  # Include this variable in the target dependencies to enable HiCR in your application
  HiCRBuildDep = dependency('HiCR', required: true)


.. note:: 
  For the last approach, the HiCR installation folder must be in the :code:`PKG_CONFIG_PATH` enviroment variable.

  

Implementing HiCR
===================

To create a backend, developers implement a subset of HiCR's core abstract classes to expose the functionality provided by the underlying technology / device. For example, to create a new backend which implements the communication manager class, you need to create a file with a class definition as follows:

..  code-block:: C++

  // Including the abstract class header file
  #include <hicr/core/instanceManager.hpp>

  class myCommManager final : public HiCR::CommunicationManager
  {
  
    public:

    myCommManager(argType myArg) : HiCR::CommunicationManager()
    {
      // ... Backend-specific constructor
    }

    // Backend-specific method overrides and attributes
  }

All the pure virtual functions defined in the HiCR abstract class need to be implemented and overriden by the backend. These functions represent common responsibilities shared by all backends of the same class and are the mechanism by which HiCR can expose a unified API for multiple devices. The backend programmer may as well add new functions and class attributes as necessary.
