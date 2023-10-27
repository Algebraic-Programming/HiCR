testSuite = [ 'examples', 'taskr', 'resourceList' ]

if 'sharedMemory' in enabledBackends
	resourceList = executable('resourceList', [ 'source/main.cpp'], dependencies: [ taskrBuildDep, hicrSharedMemoryBackendDep ])
	test('resourceList', resourceList, args : [ '4', '100', '0', '1', '2', '3' ], suite: testSuite)
endif

