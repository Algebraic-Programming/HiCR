testSuite = [ 'examples', 'taskr', 'abcTasks' ]

sequential = executable('sequential', [ 'source/sequential.cpp'],dependencies: [ taskrBuildDep, hicrSequentialBackendDep ])

if get_option('buildTests')
  test('sequential', sequential, args : [ ], suite: testSuite )
endif

if 'sharedMemory' in enabledBackends
	sharedMemory = executable('sharedMemory', [ 'source/sharedMemory.cpp'], dependencies: [ taskrBuildDep, hicrSharedMemoryBackendDep])

	if get_option('buildTests')
	  test('sharedMemory', sharedMemory, args : [ ], suite: testSuite )
	endif
endif
