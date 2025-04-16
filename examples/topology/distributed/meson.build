testSuite = [ 'examples', 'hicr', 'toplogy', 'distributed' ]
test_timeout = 60

if 'mpi' in enabledBackends and 'pthreads' in enabledBackends and 'hwloc' in enabledBackends
	mpi = executable('mpi', [ 'source/mpi.cpp'], dependencies: hicrBuildDep, include_directories: ['source'] )

	if get_option('buildTests')
	  test('mpi', mpirunExecutable, args : [ '-n', '4', '--oversubscribe', mpi.full_path() ], timeout: test_timeout, suite: testSuite )
	endif
endif
