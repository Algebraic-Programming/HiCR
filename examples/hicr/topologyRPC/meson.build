testSuite = [ 'examples', 'hicr', 'toplogyRPC' ]
test_timeout = 60

if 'mpi' in enabledBackends and 'host/pthreads' in enabledBackends and 'host/hwloc' in enabledBackends
	mpi = executable('mpi', [ 'source/mpi.cpp'], dependencies: hicrBuildDep, include_directories: ['source'] )

	if get_option('buildTests')
	  test('mpi', mpirunExecutable, args : [ '-n', '4', mpi.full_path() ], timeout: test_timeout, suite: testSuite )
	endif
endif
