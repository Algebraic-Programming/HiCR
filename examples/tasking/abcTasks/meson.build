testSuite = [ 'examples', 'tasking' ]

if 'hwloc' in enabledBackends and 'pthreads' in enabledBackends and 'boost' in enabledBackends
	abcTasks = executable('abcTasks', [ 'source/main.cpp'], dependencies: hicrBuildDep )

	if get_option('buildTests')
	  test('abcTasks', abcTasks, args : [ ], suite: testSuite )
	endif
endif
