#!/bin/bash

#SBATCH --job-name=hicr_CI_build_arm
#SBATCH --output=hicr_CI_build_arm_output.txt
#SBATCH --error=hicr_CI_build_arm_error.txt
#SBATCH --ntasks=1
#SBATCH --time=00:02:00
#SBATCH --partition=TaiShanV110

HICR_BUILD_BACKENDS='hwloc,pthreads,mpi,lpf'
HICR_BUILD_FRONTENDS='tasking,RPCEngine,channel,objectStore'

HICR_BUILD_OMPI_VERSION='4.1.7a1'

# The -already set up- environment provides the hicr dependencies: meson, googletest, hwloc, openmpi cluster-specific paths
spack env activate hicr-arm
if [ $? -ne 0 ]; then
	echo "Spack environment not set up correctly. This should be independent of HiCR builds. Please contact maintainers"
	exit 1
fi

# Some bug in Spack's behavior requires this to be explicitly loaded, even though it is part of the environment...
spack load openmpi@$HICR_BUILD_OMPI_VERSION
if [ $? -ne 0 ]; then
	echo "Spack could not load OpenMPI correctly. This should be independent of HiCR builds. Please contact maintainers"
	exit 1
fi

echo "Cleaning up..."
rm -rf build

# Could not reliably provide LFP via Spack, so we use this little snippet as we do on the other CI jobs
echo "Pulling LPF"
git submodule update --init
echo "Compiling LPF..."
mkdir extern/lpf/build; pushd extern/lpf/build; ../bootstrap.sh ; make -j24; make install || true; popd

export LD_LIBRARY_PATH=$PWD/extern/lpf/build/lib:$LD_LIBRARY_PATH
export CPATH=$PWD/extern/lpf/build/include:$CPATH
export PATH=$PWD/extern/lpf/build/bin:$PATH
export LIBRARY_PATH=$PWD/extern/lpf/build/lib:$LIBRARY_PATH

# Actual HiCR set up and build, now that the dependencies are all set
echo "Setting up..."
mkdir build

meson setup build -Dbackends=$HICR_BUILD_BACKENDS -Dfrontends=$HICR_BUILD_FRONTENDS -DbuildTests=true -DbuildExamples=true -DcompileWarningsAsErrors=false -Dbuildtype=release

if [ $? -ne 0 ]; then
	echo "Setup process failed. Please consult relevant logs."
	exit 1
else
	echo "Setup succeeded!"
fi

echo "Building..."
meson compile -C build

if [ $? -ne 0 ]; then
	echo "Build process failed. Please consult relevant logs."
	exit 1
else
	echo "Build succeeded!"
fi

spack env deactivate

exit 0

