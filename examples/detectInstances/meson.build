testSuite = [ 'examples', 'detectInstances', 'local' ]
test_timeout = 60

if 'hwloc' in enabledBackends and 'pthreads' in enabledBackends
	pthreads = executable('pthreads', [ 'source/pthreads.cpp'], dependencies: hicrBuildDep )

	if get_option('buildTests')
	  test('pthreads', pthreads, args : [ '10' ], timeout: test_timeout, suite: testSuite )
	endif
endif