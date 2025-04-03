Tasking: ABC Tasks
==================

A simple example for creating a task dependency-based runtime system using HiCR.
This example uses the :ref:`pthreads backend` for creating workers and :ref:`boost backend` for creating execution states.

It creates and adds tasks in an arbitrary order. However, given the dependencies should force the output to always be an ordered sequence of A->B->C. This example makes sure TaskR is handling these dependencies correctly.

