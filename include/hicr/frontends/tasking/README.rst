.. _tasking:

***********************
Tasking
***********************

Tasking frontend provides basic building blocks to create a task-based runtime system.

The frontend requires specifying two, possibly distinct, Compute Managers: one for the worker objects and another for the tasks.

The tasks are represented as the HiCR Execution States and the Workers the Processing Units.

Tasking provides basic support for stateful tasks with settable callbacks to notify when the task changes state, e.g., from executing to finished.
As well as a set of workers which can be suspended and resume.

TODO: explain the Task and Worker class more in detail