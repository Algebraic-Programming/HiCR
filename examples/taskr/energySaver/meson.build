testSuite = [ 'examples', 'hicr', 'energySaver' ]

if 'sharedMemory' in enabledBackends
	energySaver = executable('energySaver', [ 'source/main.cpp'], dependencies: [ taskrBuildDep, hicrSharedMemoryBackendDep])
	test('energySaver', energySaver, args : [ '1', '0' ], suite: testSuite)
endif

