testSuite = [ 'examples', 'memcpy', 'local' ]
test_timeout = 60

if 'hwloc' in enabledBackends and 'pthreads' in enabledBackends
	pthreads = executable('pthreads', [ 'source/pthreads.cpp'], dependencies: hicrBuildDep )

	if get_option('buildTests')
	  test('pthreads', pthreads, args : [ ], timeout: test_timeout, suite: testSuite )
	endif
endif

if 'acl' in enabledBackends and 'hwloc' in enabledBackends
	acl = executable('acl', [ 'source/acl.cpp'], dependencies: hicrBuildDep)

	if get_option('buildTests')
	  test('acl', acl, args : [ ], is_parallel: false, timeout: test_timeout, suite: testSuite ) 
	endif
endif

if 'opencl' in enabledBackends and 'hwloc' in enabledBackends
	opencl = executable('opencl', [ 'source/opencl.cpp'], dependencies: hicrBuildDep)

	if get_option('buildTests')
	  test('opencl', opencl, args : [ ], is_parallel: false, timeout: test_timeout, suite: testSuite ) 
	endif
endif

