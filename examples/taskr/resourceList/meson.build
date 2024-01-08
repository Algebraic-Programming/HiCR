testSuite = [ 'examples', 'taskr' ]

if 'sharedMemory/hwloc' in enabledBackends and 'sharedMemory/pthreads' in enabledBackends
	resourceList = executable('resourceList', [ 'source/main.cpp'], dependencies: [ taskrBuildDep, hicrSharedMemoryHWLocBackendDep, hicrSharedMemoryPthreadsBackendDep ])

	if get_option('buildTests')
	  test('resourceList', resourceList, args : [ '4', '100', '0', '1', '2', '3' ], suite: testSuite)
	endif
endif

