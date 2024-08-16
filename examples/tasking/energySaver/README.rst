Energy Saver
===============

A simple example that showcases the ajust the number of active workers in response to a lack of work, thereby increasing energy efficiency. 

This example runs in three phases:
* Phase 1: Run many computationally intensive tasks. Here, all CPU cores must be in 100% use
* Phase 2: Run a single passive task. Here, all cores must be mostly in 0% use
* Phase 3: Repeat phase 1.

If the core usage follows the pattern described above, it means Taskr is successfully decreasing CPU use based on computational demands. This is an improvement to keeping workers at 100% CPU use by constantly polling for new tasks.

