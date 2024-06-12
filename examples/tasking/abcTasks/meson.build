testSuite = [ 'examples', 'tasking' ]

if 'host/hwloc' in enabledBackends and 'host/pthreads' in enabledBackends
	abcTasks = executable('abcTasks', [ 'source/main.cpp'], dependencies: hicrBuildDep )

	if get_option('buildTests')
	  test('abcTasks', abcTasks, args : [ ], suite: testSuite )
	endif
endif
