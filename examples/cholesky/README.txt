Cholesky Decomposition Sandbox
=======================================

A sandbox to test new approaches to Cholesky patterns and new runtime system research  

Compilation
------------

mkdir build
cd build
meson setup .. 
ninja

Running
-----------

./run_all.sh N bs 

where:
 * N = Matrix Size
 * bs = Block Size (where applicable)
 * [--check] to verify residual

example: 
./run_all.sh 2048 128
./run_all.sh 2048 128 --check




