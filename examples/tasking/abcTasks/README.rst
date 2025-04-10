.. _abcTasks:

Tasking: ABC Tasks
==================

A simple example for creating a task dependency-based runtime system using HiCR.
This example utilizes :ref:`pthreads backend` to create processing units (workers) and :ref:`boost backend` to create execution states (tasks).

It creates and adds tasks in an arbitrary order. However, given the dependencies should force the output to always be an ordered sequence of A->B->C. 
This test example makes sure the runtime is handling the tasks dependencies correctly.
