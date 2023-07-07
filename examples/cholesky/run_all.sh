#!/bin/bash

# Configuration variables
n=4096
bs=64
num_cores=44
verify=true
export OMP_PLACES=threads
export OMP_PROC_BIND=false
export OMP_NUM_THREADS=${num_cores}
export OPENBLAS_NUM_THREADS=1

# Argument preparation
checkFlag=""
if [[ "${verify}" == "true" ]]; then checkFlag="--check"; fi
args="${n} ${bs} ${checkFlag}"

#OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=${num_cores} build/cholesky_unblocked ${args}
#build/cholesky_blocked_ompss ${args}
#build/cholesky_blocked_omp_threads ${args}
#build/cholesky_blocked_omp_tasks ${args}
build/cholesky_blocked_taskr_static ${args}
#build/cholesky_blocked_taskr_dynamic ${args}
