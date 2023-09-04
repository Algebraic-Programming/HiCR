#!/bin/bash

if [[ $# -lt 2 ]]; then
    echo "Usage: $0 <N> <bs> [--check]]"
    echo "Where:"
    echo "  N: (Square) Matrix size"
    echo "  bs: Block size" 
    echo "  check: Performs a correctness check"
    exit 1
fi

function check()
{
  if [ ! $? -eq 0 ]; then 
    echo "Error running example."
    exit -1 
  fi
}

# Configuration variables
n=${1}
bs=${2}
verify="false"
if [[ $# -eq 3 ]]; then verify="true"; fi
num_cores=`nproc`
export OMP_PLACES=threads
export OMP_PROC_BIND=false
export OMP_NUM_THREADS=${num_cores}
export OPENBLAS_NUM_THREADS=1

# Argument preparation
checkFlag=""
if [[ "${verify}" == "true" ]]; then checkFlag="--check"; fi
args="${n} ${bs} ${checkFlag}"

# Setting up build dir
buildDir=../../../build/examples/taskr/cholesky/

OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=${num_cores} ${buildDir}/cholesky_unblocked ${args}; check 
${buildDir}/cholesky_blocked_omp_threads ${args}; check
${buildDir}/cholesky_blocked_omp_tasks ${args}; check
${buildDir}/cholesky_blocked_taskr_static ${args}; check
${buildDir}/cholesky_blocked_taskr_dynamic ${args}; check
