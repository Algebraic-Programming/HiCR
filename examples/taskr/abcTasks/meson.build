testSuite = [ 'examples', 'taskr', 'abcTasks' ]

sequential = executable('sequential', [ 'source/sequential.cpp'],dependencies: [ taskrBuildDep, hicrSequentialBackendDep ])
get_option('buildTests') ? test('sequential', sequential, args : [ ], suite: testSuite ) :

if 'sharedMemory' in enabledBackends
	sharedMemory = executable('sharedMemory', [ 'source/sharedMemory.cpp'], dependencies: [ taskrBuildDep, hicrSharedMemoryBackendDep])
	get_option('buildTests') ? test('sharedMemory', sharedMemory, args : [ ], suite: testSuite ) :
endif
