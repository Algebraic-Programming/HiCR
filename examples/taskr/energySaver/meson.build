testSuite = [ 'examples', 'taskr', 'energySaver' ]

if 'sharedMemory' in enabledBackends
	energySaver = executable('energySaver', [ 'source/main.cpp'], dependencies: [ taskrBuildDep, hicrSharedMemoryBackendDep])
	test('energySaver', energySaver, args : [ '10', '1', '100' ], suite: testSuite)
endif

