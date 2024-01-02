testSuite = [ 'examples', 'taskr', 'energySaver' ]

if 'sharedMemory' in enabledBackends
	energySaver = executable('energySaver', [ 'source/main.cpp'], dependencies: [ taskrBuildDep, hicrSharedMemoryBackendDep])

	if get_option('buildTests')
	  test('energySaver', energySaver, args : [ '10', '1', '100' ], suite: testSuite)
	endif
endif

