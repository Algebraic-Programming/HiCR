Fibonacci
============

A simple example for computing the Fibonacci sequence with a task dependency-based runtime system using HiCR.

Each call to the Fibonacci(n) function creates two tasks to compute Fibonacci(n-1) and Fibonacci(n-2), creates dependencies with them, and pauses itself until these tasks have been completed.