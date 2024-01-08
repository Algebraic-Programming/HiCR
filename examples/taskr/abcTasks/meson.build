testSuite = [ 'examples', 'taskr' ]

if 'sharedMemory/hwloc' in enabledBackends and 'sharedMemory/pthreads' in enabledBackends
	abcTasks = executable('abcTasks', [ 'source/main.cpp'], dependencies: [ taskrBuildDep, hicrSharedMemoryHWLocBackendDep, hicrSharedMemoryPthreadsBackendDep] )

	if get_option('buildTests')
	  test('abcTasks', abcTasks, args : [ ], suite: testSuite )
	endif
endif
