testSuite = [ 'examples', 'channels', 'spsc']
test_timeout = 180

exampleBuildIncludes = include_directories([
   'source/include'
  ])
  
if 'sharedMemory/hwloc' in enabledBackends and 'sharedMemory/pthreads' in enabledBackends
    sharedMemoryDependencies = [ hicrBuildDep, hicrSharedMemoryHWLocBackendDep, hicrSharedMemoryPthreadsBackendDep] 
	sharedMemory = executable('sharedMemory', [ 'source/sharedMemory.cpp' ], dependencies: sharedMemoryDependencies, include_directories: [exampleBuildIncludes])

	if get_option('buildTests')
	 test('sharedMemory', sharedMemory, args : [ '2' ], timeout: test_timeout, suite: testSuite )
	 test('sharedMemory', sharedMemory, args : [ '3' ], timeout: test_timeout, suite: testSuite )
	endif
endif

if 'mpi' in enabledBackends and 'sharedMemory/hwloc' in enabledBackends
	mpi = executable('mpi', [ 'source/mpi.cpp' ], dependencies: [ hicrBuildDep, hicrMPIBackendDep, hicrSharedMemoryHWLocBackendDep], include_directories: [exampleBuildIncludes] )

	if get_option('buildTests')
	  test('mpi', mpirunExecutable, args : [ '-n', '2', mpi.full_path(), '3' ], timeout: test_timeout, suite: testSuite )
    endif
endif

if 'lpf' in enabledBackends and 'sharedMemory/hwloc' in enabledBackends
	lpf = executable('lpf', [ 'source/lpf.cpp' ], dependencies: [ hicrBuildDep, hicrLPFBackendDep, hicrSharedMemoryHWLocBackendDep], include_directories: [exampleBuildIncludes] )

	if get_option('buildTests')
	  test('lpf', lpfrunExecutable, args : [ '-np', '2', '-engine', 'ibverbs', lpf.full_path(), '3' ], timeout: test_timeout, suite: testSuite )
	endif
endif
