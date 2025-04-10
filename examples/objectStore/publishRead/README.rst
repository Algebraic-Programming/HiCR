.. _ospubread:

Object Store: Publish Read
==========================

This example showcases the object store capabilities with a multiples HiCR instances in the system. The code is structured as follows:

* :code:`source/` contains the different variants of this example corresponding to different backends

    * :code:`lpf.cpp` corresponds to the :ref:`lpf backend` backends implementation. As instance manager, the :ref:`mpi backend` one is used

Data Object creation 
--------------------

First, the root instance is the owner of 2 data objects: 

.. code-block:: c++
  
  // Allocate memory for our blocks
  auto myBlockSlot  = memoryManager.allocateLocalMemorySlot(objectStore.getMemorySpace(), BLOCK_SIZE);
  auto myBlockSlot2 = memoryManager.allocateLocalMemorySlot(objectStore.getMemorySpace(), BLOCK_SIZE);

  char *myBlockData  = (char *)myBlockSlot->getPointer();
  char *myBlockData2 = (char *)myBlockSlot2->getPointer();

  auto myBlock  = objectStore.createObject(myBlockSlot, 0);
  auto myBlock2 = objectStore.createObject(myBlockSlot2, 1);

  // Publish the first block
  objectStore.publish(myBlock);

  // Publish the second block
  objectStore.publish(myBlock2);

Data object handle serialization and broadcast
----------------------------------------------

Then it sends over channels the `handle` of each data object to the other instances:

.. code-block:: C++
  
  // Serialize the blocks
  auto handle1 = objectStore.serialize(*myBlock);

  // Create buffers to send the serialized blocks, copy the serialized blocks into the buffers
  uint8_t *serializedBlock  = new uint8_t[sizeof(HiCR::objectStore::handle)];

  std::memcpy(serializedBlock, &handle1, sizeof(HiCR::objectStore::handle));
  auto sendSlot  = memoryManager.registerLocalMemorySlot(memorySpace, serializedBlock, sizeof(HiCR::objectStore::handle));

  // Send the block information to the reader via the channel
  producer.push(sendSlot);

  while (!producer.isEmpty()) producer.updateDepth();

  // Fence to ensure all blocks are sent
  communicationManager.fence(CHANNEL_TAG);

Data object handle deserialization
----------------------------------------------

Then, on the reader side the `handle` is recevied and then deserialized: 

.. code-block:: C++

  // Getting internal pointer to the received message
  auto payloadBufferPtr = payloadBufferSlot->getPointer();

  // Copy the received handle
  std::memcpy(&handle1, (uint8_t *)payloadBufferPtr , sizeof(HiCR::objectStore::handle));

  // Deserialize the handle
  auto dataObject1 = objectStore.deserialize(handle1);


Get data object
----------------

The data object can be retrieved:

.. code-block:: c++

  auto objSlot1 = objectStore.get(*dataObject1);

  // One-sided fence to ensure this block is received
  objectStore.fence(dataObject1);

The same is done for the second data object.

Data object destruction
-----------------------

Finally, the data object are destroyed:

.. code-block:: c++
  
  objectStore.destroy(*dataObject1);

The output should like the following:

.. code-block:: bash

  Reader: Received block 1: Test
  Reader: Received block 2: This is another block
