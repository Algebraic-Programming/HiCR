.. _objectstore:

***********************
Object Store
***********************

Object store is a communication mechanism complementary to the one offered by :ref:`channels`. Object store is suitable for less frequent communications that involve large data exchange.

Any HiCR instance can create a :code:`data object` and publish it. That is, it makes the object available all the other instances. An object can be retrieved by providing it's :code:`handle`: the object's metadata.

To use the object store, the following steps are required:

* Create a data object, out of a memory slot, and publish it.
* Acquire the data object's handle from the object store, via serialization.
* Transfer the handle to the other instance. (This can be done via e.g., a channel.)
* The other instance can then deserialize the handle and access the data object via :code:`get()`.

Creating and publishing a data object
----------------------------------------

The following snippet shows how to create a data object and publish it. The object is created out of a local memory slot. The object is then published to the object store, making it available to other instances.

..  code-block:: C++

   #define OBJECT_ID 1
   #define SIZE      1024

   auto mySlot   = memoryManager.createLocalMemorySlot(SIZE);
   auto myObject = objectStore.createObject(mySlot, OBJECT_ID);

   objectStore.publish(myObject);

Data object serialization and transfer
-------------------------------------------

The following snippet shows how to serialize the data object's handle and transfer it to another instance. The handle can then be deserialized to access the data object.

..  code-block:: C++

   // Serializing the handle
   auto serializedHandle = objectStore.serialize(myObject);

   // Transferring the handle to another instance
   auto sendSlot = memoryManager.createLocalMemorySlot(sizeof(serializedHandle));
   memcpy(sendSlot->getPointer(), &serializedHandle, sizeof(serializedHandle));

   // Assuming we have established a channel with the other instance, then we fence if needed
   channel.push(sendSlot);

Receiving and deserializing the data object
------------------------------------------------

To complete the process, the other instance must receive the serialized handle and deserialize it to access the data object. After that, the data object can be retrieved via get().

..  code-block:: C++

   HiCR::objectStore::handle receivedHandle;

   // Receiving the serialized handle
   auto receivedBuffer = channelPayload->getPointer();
   auto res            = channel.peek();
   channel.pop();

   std::memcpy(&receivedHandle, receivedBuffer + res[0], sizeof(receivedHandle));

   auto dataObject = objectStore.deserialize(receivedHandle);

   auto dataSlot = objectStore.get(dataObject);

   // Fence to ensure the data is available
   objectStore.fence(dataObject);


Destroying a data object
--------------------------------

The data object can be destroyed when it is no longer needed. This can be done via the destroy() method.

..  code-block:: C++

   // Destroying the data object, from the above example
   auto dataObject = objectStore.deserialize(receivedHandle);

   /* Do something with the data object */

   objectStore.destroy(*dataObject);

API reference available: `Doxygen <../../../doxygen/html/dir_909ac9aaf85ef872017c2f1530af546a.html>`_