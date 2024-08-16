testSuite = [ 'examples', 'tasking' ]

if 'host/hwloc' in enabledBackends and 'host/pthreads' in enabledBackends
	energySaver = executable('energySaver', [ 'source/main.cpp'], dependencies: hicrBuildDep)

	if get_option('buildTests')
	  test('energySaver', energySaver, args : [ '10', '1', '100' ], suite: testSuite)
	endif
endif

