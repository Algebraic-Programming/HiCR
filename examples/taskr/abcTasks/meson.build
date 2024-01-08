testSuite = [ 'examples', 'taskr' ]

if 'hwloc' in enabledBackends and 'pthreads' in enabledBackends
	abcTasks = executable('abcTasks', [ 'source/main.cpp'], dependencies: [ taskrBuildDep, hicrHWLocBackendDep, hicrPthreadsBackendDep] )

	if get_option('buildTests')
	  test('abcTasks', abcTasks, args : [ ], suite: testSuite )
	endif
endif
