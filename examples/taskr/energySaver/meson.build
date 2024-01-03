testSuite = [ 'examples', 'taskr', 'energySaver' ]

if 'sharedMemory/hwloc' in enabledBackends and 'sharedMemory/pthreads' in enabledBackends
	energySaver = executable('energySaver', [ 'source/main.cpp'], dependencies: [ taskrBuildDep, hicrSharedMemoryHWLocBackendDep, hicrSharedMemoryPthreadsBackendDep])

	if get_option('buildTests')
	  test('energySaver', energySaver, args : [ '10', '1', '100' ], suite: testSuite)
	endif
endif

