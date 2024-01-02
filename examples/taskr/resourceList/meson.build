testSuite = [ 'examples', 'taskr', 'resourceList' ]

if 'sharedMemory' in enabledBackends
	resourceList = executable('resourceList', [ 'source/main.cpp'], dependencies: [ taskrBuildDep, hicrSharedMemoryBackendDep ])

	if get_option('buildTests')
	  test('resourceList', resourceList, args : [ '4', '100', '0', '1', '2', '3' ], suite: testSuite)
	endif
	
endif

