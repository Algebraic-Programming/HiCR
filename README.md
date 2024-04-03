[![Latest Release](https://gitlab.huaweirc.ch/zrc-von-neumann-lab/runtime-system-innovations/hicr/-/badges/release.svg)](https://gitlab.huaweirc.ch/zrc-von-neumann-lab/runtime-system-innovations/hicr/-/releases) [![pipeline status](https://gitlab.huaweirc.ch/zrc-von-neumann-lab/runtime-system-innovations/hicr/badges/master/pipeline.svg)](https://gitlab.huaweirc.ch/zrc-von-neumann-lab/runtime-system-innovations/hicr/-/commits/master) [![coverage report](https://gitlab.huaweirc.ch/zrc-von-neumann-lab/runtime-system-innovations/hicr/badges/master/coverage.svg)](https://gitlab.huaweirc.ch/zrc-von-neumann-lab/runtime-system-innovations/hicr/-/commits/master) 
 
# HiCR (HiSilicon Common Runtime)

This is a header only base layer for the FF3.0 ecosystem. It contains a set of building blocks for the construction of runtime systems that can operate on many distributed and heterogeneous systems using a unified API.

User Manual: [zrc-von-neumann-lab.pages.huaweirc.ch/runtime-system-innovations/hicr](https://zrc-von-neumann-lab.pages.huaweirc.ch/runtime-system-innovations/hicr/) (Blue Zone access required)

## Quickstart Guide

Step 0) Make sure the following libraries and tools are installed:

```
C++ compiler with suppport for C++20 (e.g., g++ >=11.0)
meson (version >1.0.0)
minja (version >1.0.0)
OpenMPI (version >= 5.0.2)
python (version >= 3.9)
hwloc
gtest
```

Step 1) [Blue Zone] Clone HiCR branch recursively

```
git clone https://gitlab.huaweirc.ch/zrc-von-neumann-lab/runtime-system-innovations/hicr.git --recursive
```

Step 2) Create 'build' folder and get inside

```
mkdir hicr/build`
cd hicr/build
```

Step 3) Configure Meson build environment with support for MPI, Pthreads, and HWloc

```
meson .. -DbuildExamples=true -Dbackends=mpi,hwloc,pthreads
```

Step 4) Compile

```
ninja
```

Step 5) (Optional) Run tests

```
meson tests
```

## Running Examples

Run [ABC Tasks](examples/tasking/abcTasks/source/include/abcTasks.hpp) example (single instance)

```
examples/tasking/abcTasks/abcTasks
```

Run [Single-Producer Single-Consumer Channel](examples/channels/fixedSize/spsc/include) example using 2 MPI processes (consumer and producer) and a channel size of 3.

```
mpirun -n 2 examples/channels/fixedSize/spsc/mpi 3
```

