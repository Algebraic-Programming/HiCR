Object Store - Single Instance
=====================

This example showcases the Object Store capabilities with a single HiCR Instance in the system. The code is structured as follows:

* :code:`source/` contains the different variants of this example corresponding to different backends

    * :code:`main.cpp` corresponds to the :ref:`hwloc` and :ref:`pthreads` backends implementation
   
First, we create a DataObject from a Local Memory Slot:

.. code-block:: C++

  // Allocate Local Memory Slot
  auto myBlockSlot = memoryManager->allocateLocalMemorySlot(memSpace, 4096);
  
  // Initialize the block with a ASCII representation of 'R'
  char *myBlockData = (char *)myBlockSlot->getPointer();
  myBlockData[0] = 82;

  // Publish the block with arbitrary ID 1
  std::shared_ptr<HiCR::objectStore::DataObject> myBlock = 
  std::make_shared<HiCR::objectStore::DataObject>(objectStoreManager.createObject(myBlockSlot, 1));


Then, we publish it. That is, make it avaialable for retrieval: 

.. code-block:: C++

  objectStoreManager.publish(myBlock);

Then, we get it: 

.. code-block:: C++

  // Get the raw pointers through the block
  char *myBlockData1 = (char *)objectStoreManager.get(*myBlock)->getPointer();

The output should be:

.. code-block:: bash

  Block 1: R

We do it again by modifying the buffer content first, and then testing out copying capabilities. The complete output should be:

.. code-block:: bash

  Block 1: R
  Block 2: S
  Copy of a Custom Block: Test