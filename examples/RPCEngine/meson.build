testSuite = [ 'examples', 'hicr', 'RPCEngine' ]
test_timeout = 60

if 'mpi' in enabledBackends
	mpi = executable('mpi', [ 'source/mpi.cpp'], dependencies: hicrBuildDep, include_directories: ['source'] )

	if get_option('buildTests')
	  test('mpi', mpi, timeout: test_timeout, suite: testSuite )
	endif
endif
