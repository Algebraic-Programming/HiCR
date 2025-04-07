.. _contributing:

***********************
Contributing
***********************

(To be written)
   
.. _docker:

Docker
***********************

(To be written)

.. _backendDevelopment:

Backend Development
***********************

To create a backend, developers implement a subset of HiCR's core abstract classes to expose the functionality provided by the underlying technology / device. For example, to create a new backend which implements the Communication Manager class, you need to create a file with a class definition as follows:

..  code-block:: C++
 :caption: myBackend.hpp

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
