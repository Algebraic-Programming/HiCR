testSuite = [ 'examples', 'memcpy', 'local' ]
test_timeout = 60

if 'hwloc' in enabledBackends and 'pthreads' in enabledBackends
	pthreads = executable('pthreads', [ 'source/pthreads.cpp'], dependencies: hicrBuildDep )

	if get_option('buildTests')
	  test('pthreads', pthreads, args : [ ], timeout: test_timeout, suite: testSuite )
	endif
endif

if 'ascend' in enabledBackends and 'hwloc' in enabledBackends
	ascend = executable('ascend', [ 'source/ascend.cpp'], dependencies: hicrBuildDep)

	if get_option('buildTests')
	  test('ascend', ascend, args : [ ], is_parallel: false, timeout: test_timeout, suite: testSuite ) 
	endif
endif
