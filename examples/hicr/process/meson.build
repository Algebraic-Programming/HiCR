testSuite = [ 'examples', 'hicr', 'process']
test_timeout = 180

if 'mpi' in enabledBackends
	mpi = executable('mpi', [ 'source/mpi.cpp'], dependencies: [ hicrBuildDep, hicrMPIBackendDep], include_directories: [exampleBuildIncludes] )
	test('mpi', mpirunExecutable, args : [ '-n', '4', mpi.full_path() ], timeout: test_timeout, suite: testSuite )
endif
