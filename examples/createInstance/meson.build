testSuite = [ 'examples', 'createInstance', 'local' ]
test_timeout = 60

includeDirs = include_directories(['include'])

if 'hwloc' in enabledBackends and 'pthreads' in enabledBackends
	pthreads = executable('pthreads', [ 'source/pthreads.cpp'], include_directories: includeDirs, dependencies: hicrBuildDep )

	if get_option('buildTests')
	  test('pthreads', pthreads, args : [ '10' ], timeout: test_timeout, suite: testSuite )
	endif
endif