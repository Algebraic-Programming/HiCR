testSuite = [ 'examples', 'taskr' ]

if 'hwloc' in enabledBackends and 'pthreads' in enabledBackends
	energySaver = executable('energySaver', [ 'source/main.cpp'], dependencies: hicrBuildDep)

	if get_option('buildTests')
	  test('energySaver', energySaver, args : [ '10', '1', '100' ], suite: testSuite)
	endif
endif

