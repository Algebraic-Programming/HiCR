testSuite = [ 'examples', 'tasking' ]

if 'host/hwloc' in enabledBackends and 'host/pthreads' in enabledBackends
	resourceList = executable('resourceList', [ 'source/main.cpp'], dependencies: hicrBuildDep)

	if get_option('buildTests')
	  test('resourceList', resourceList, args : [ '4', '100', '0', '1', '2', '3' ], suite: testSuite)
	endif
endif

