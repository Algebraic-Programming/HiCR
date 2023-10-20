testSuite = [ 'examples', 'hicr', 'memcpy' ]

sequential = executable('sequential', [ 'source/sequential.cpp'], dependencies: [ hicrBuildDep, hicrSequentialBackendDep])
get_option('buildTests') ? test('sequential', sequential, args : [ ], suite: testSuite ) :


if 'sharedMemory' in enabledBackends
	sharedMemory = executable('sharedMemory', [ 'source/sharedMemory.cpp'], dependencies: [ hicrBuildDep, hicrSharedMemoryBackendDep])
	get_option('buildTests') ? test('sharedMemory', sharedMemory, args : [ ], suite: testSuite ) :
endif

if 'ascend' in enabledBackends 
	ascend = executable('ascend', [ 'source/ascend.cpp'], dependencies: [ hicrBuildDep, hicrAscendBackendDep])
	get_option('buildTests') ? test('ascend', sharedMemory, args : [ ], suite: testSuite ) :
endif
