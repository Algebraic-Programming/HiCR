testSuite = [ 'examples', 'runtime' ]
test_timeout = 60

if 'mpi' in enabledBackends and 'pthreads' in enabledBackends and 'hwloc' in enabledBackends
	mpi = executable('mpi', [ 'source/main.cpp'], dependencies: hicrBuildDep, include_directories: ['source'] )

	if get_option('buildTests')
	  test('mpi', mpirunExecutable, args : [ '-n', '8', mpi.full_path() ], timeout: test_timeout, suite: testSuite )
	endif
endif
