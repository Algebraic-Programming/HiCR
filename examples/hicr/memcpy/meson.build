testSuite = [ 'examples', 'hicr', 'memcpy' ]

sequential = executable('sequential', [ 'source/sequential.cpp'], dependencies: [ hicrBuildDep, hicrSequentialBackendDep])
test('sequential', sequential, args : [ ], suite: testSuite ) 


if 'sharedMemory' in enabledBackends
	sharedMemory = executable('sharedMemory', [ 'source/sharedMemory.cpp'], dependencies: [ hicrBuildDep, hicrSharedMemoryBackendDep])
	test('sharedMemory', sharedMemory, args : [ ], suite: testSuite )
endif

if 'lpf' in enabledBackends
	lpf = executable('lpf', [ 'source/lpf.cpp'], dependencies: [ hicrBuildDep, hicrLPFBackendDep])
	test('lpf', lpfrunExecutable, args : [ '-np', '2', '-engine', 'ibverbs', lpf.full_path() ], suite: testSuite ) 
endif


if 'ascend' in enabledBackends 
	ascend = executable('ascend', [ 'source/ascend.cpp'], dependencies: [ hicrBuildDep, hicrAscendBackendDep])
	test('ascend', ascend, args : [ ], is_parallel: false, suite: testSuite ) 
endif
