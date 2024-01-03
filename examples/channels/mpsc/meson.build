testSuite = [ 'examples', 'channels', 'mpsc']
test_timeout = 180

exampleBuildIncludes = include_directories([
   'source/include'
  ])
  
if 'sharedMemory/hwloc' in enabledBackends and 'sharedMemory/pthreads' in enabledBackends
    sharedMemoryDependencies = [ hicrBuildDep, hicrSharedMemoryHWLocBackendDep, hicrSharedMemoryPthreadsBackendDep] 
	sharedMemory = executable('sharedMemory', [ 'source/sharedMemory.cpp' ], dependencies: sharedMemoryDependencies, include_directories: [exampleBuildIncludes])

	if get_option('buildTests')
	  test('sharedMemory', sharedMemory, args : [ '3', '15' ], timeout: test_timeout, suite: testSuite )
	endif
endif

if 'mpi' in enabledBackends
	mpi = executable('mpi', [ 'source/mpi.cpp' ], dependencies: [ hicrBuildDep, hicrMPIBackendDep], include_directories: [exampleBuildIncludes] )

	if get_option('buildTests')
	test('mpi', mpirunExecutable, args : [ '-n', '16', mpi.full_path(), '3' ], timeout: test_timeout, suite: testSuite )
	endif
endif
