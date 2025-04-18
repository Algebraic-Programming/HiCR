.. _ossingle:

Object Store: Single Instance
=============================

This example showcases the object store capabilities with a single HiCR instance in the system. The code is structured as follows:

* :code:`source/` contains the different variants of this example corresponding to different backends

    * :code:`main.cpp` corresponds to the :ref:`hwloc backend` and :ref:`pthreads backend` backends implementation
   
Data object creation 
---------------------

First, we create a DataObject from a local memory slot:

.. code-block:: C++

  // Allocate local memory slot
  auto myBlockSlot = memoryManager->allocateLocalMemorySlot(memSpace, 4096);
  
  // Initialize the block with a ASCII representation of 'R'
  char *myBlockData = (char *)myBlockSlot->getPointer();
  myBlockData[0] = 82;

  // Publish the block with arbitrary ID 1
  std::shared_ptr<HiCR::objectStore::DataObject> myBlock = objectStoreManager.createObject(myBlockSlot, 1);

Publish data object 
---------------------

Then, we publish it. That is, make it avaialable for retrieval: 

.. code-block:: C++

  objectStoreManager.publish(myBlock);

Get data object  
---------------------

Then, we get it: 

.. code-block:: C++

  // Get the raw pointers through the block
  char *myBlockData1 = (char *)objectStoreManager.get(*myBlock)->getPointer();

The output should be:

.. code-block:: bash

  Block 1: R

We do it again by modifying the buffer content first, and then testing out data object copying capabilities.

Destroy data object
-------------------

Finally, we destroy the data object:

.. code-block:: c++

  // Delete the data object
  objectStoreManager.destroy(*myBlock2);

The complete output should be:

.. code-block:: bash

  Block 1: R
  Block 2: S
  Copy of a Custom Block: Test
