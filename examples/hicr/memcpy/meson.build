testSuite = [ 'examples', 'hicr', 'memcpy' ]

sequential = executable('sequential', [ 'source/sequential.cpp'], dependencies: [ hicrBuildDep, hicrSequentialBackendDep])

if get_option('buildTests')
  test('sequential', sequential, args : [ ], suite: testSuite ) 
endif


if 'sharedMemory' in enabledBackends
	sharedMemory = executable('sharedMemory', [ 'source/sharedMemory.cpp'], dependencies: [ hicrBuildDep, hicrSharedMemoryBackendDep])

	if get_option('buildTests')
	  test('sharedMemory', sharedMemory, args : [ ], suite: testSuite )
	endif
endif

if 'lpf' in enabledBackends
	lpf = executable('lpf', [ 'source/lpf.cpp'], dependencies: [ hicrBuildDep, hicrLPFBackendDep])

	if get_option('buildTests')
	  test('lpf', lpfrunExecutable, args : [ '-np', '2', '-engine', 'ibverbs', lpf.full_path() ], suite: testSuite ) 
	endif
endif


if 'ascend' in enabledBackends 
	ascend = executable('ascend', [ 'source/ascend.cpp'], dependencies: [ hicrBuildDep, hicrAscendBackendDep])

	if get_option('buildTests')
	  test('ascend', ascend, args : [ ], is_parallel: false, suite: testSuite ) 
	endif
endif
