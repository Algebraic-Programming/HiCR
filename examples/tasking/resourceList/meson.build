testSuite = [ 'examples', 'tasking' ]

if 'hwloc' in enabledBackends and 'pthreads' in enabledBackends and 'boost' in enabledBackends
	resourceList = executable('resourceList', [ 'source/main.cpp'], dependencies: hicrBuildDep)

	if get_option('buildTests')
	  test('resourceList', resourceList, args : [ '4', '100', '0', '1', '2', '3' ], suite: testSuite)
	endif
endif

