.. _objectstore:

***********************
Object Store
***********************

Object Store is a communication mechanism complementary to the one offered by channels. Object Store is suitable for less frequent communications that involve large data exchange.

Any instance can create a Data Object and publish it. This makes the object available to any other instance, provided the instance acquires the object's metadata (handle).

To use the Object Store, the following steps are required:

* Create a Data Object, out of a Memory Slot, and publish it.
* Acquire the Data Object's metadata (handle) from the Object Store, via serialization.
* Transfer the handle to the other instance. (This can be done via e.g., a channel.)
* The other instance can then deserialize the handle and access the Data Object via get().

Creating and publishing a Data Object
----------------------------------------

The following snippet shows how to create a Data Object and publish it. The object is created out of a Local Memory Slot. The object is then published to the Object Store, making it available to other instances.

..  code-block:: C++

   #define OBJECT_ID 1
   #define SIZE      1024

   auto mySlot   = memoryManager.createLocalMemorySlot(SIZE);
   auto myObject = objectStore.createObject(mySlot, OBJECT_ID);

   objectStore.publish(myObject);

Data Object serialization and transfer
-------------------------------------------

The following snippet shows how to serialize the Data Object's metadata (handle) and transfer it to another instance. The handle can then be deserialized to access the Data Object.

..  code-block:: C++

   // Serializing the handle
   auto serializedHandle = objectStore.serialize(myObject);

   // Transferring the handle to another instance
   auto sendSlot = memoryManager.createLocalMemorySlot(sizeof(serializedHandle));
   memcpy(sendSlot->getPointer(), &serializedHandle, sizeof(serializedHandle));

   // Assuming we have established a channel with the other instance, then we fence if needed
   channel.push(sendSlot);

Receiving and deserializing the Data Object
------------------------------------------------

To complete the process, the other instance must receive the serialized handle and deserialize it to access the Data Object. After that, the Data Object can be retrieved via get().

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


Destroying a Data Object
--------------------------------

The Data Object can be destroyed when it is no longer needed. This can be done via the destroy() method.

..  code-block:: C++

   // Destroying the Data Object, from the above example
   auto dataObject = objectStore.deserialize(receivedHandle);

   /* Do something with the data object */

   objectStore.destroy(*dataObject);
