testSuite = [ 'examples', 'hicr', 'rpc']
test_timeout = 180

if 'mpi' in enabledBackends and 'sharedMemory/pthreads' in enabledBackends
	mpi = executable('mpi', [ 'source/mpi.cpp'], dependencies: [ hicrBuildDep, hicrMPIBackendDep, hicrSharedMemoryPthreadsBackendDep], include_directories: [exampleBuildIncludes, 'source'] )

	if get_option('buildTests')
	  test('mpi', mpirunExecutable, args : [ '-n', '4', mpi.full_path() ], timeout: test_timeout, suite: testSuite )
	endif
endif
