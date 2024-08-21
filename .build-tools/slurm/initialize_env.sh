#!/bin/bash

# This script is supposed to run on the login node and set up the environment for the build job
# that will be launched afterwards through srun/sbatch.

HICR_BUILD_SPACK_DIR=$HOME/spack

# Initialize spack to use in next steps
. $HICR_BUILD_SPACK_DIR/share/spack/setup-env.sh
if [ $? -ne 0 ]; then
	echo "Spack not set up correctly. Please contact maintainers or sysadmins"
	exit 1
fi
