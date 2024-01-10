testSuite = [ 'examples', 'hicr', 'memcpy' ]
test_timeout = 60

if 'hwloc' in enabledBackends and 'pthreads' in enabledBackends
    sharedMemoryDependencies = [ hicrBuildDep, hicrHWLocBackendDep, hicrPthreadsBackendDep ]
	sharedMemory = executable('sharedMemory', [ 'source/sharedMemory.cpp'], dependencies: sharedMemoryDependencies )

	if get_option('buildTests')
	  test('sharedMemory', sharedMemory, args : [ ], timeout: test_timeout, suite: testSuite )
	endif
endif

if 'lpf' in enabledBackends and 'hwloc' in enabledBackends 
	lpf = executable('lpf', [ 'source/lpf.cpp'], dependencies: [ hicrBuildDep, hicrLPFBackendDep, hicrHWLocBackendDep])

	if get_option('buildTests')
	  test('lpf', lpfrunExecutable, args : [ '-np', '2', '-engine', 'ibverbs', lpf.full_path() ], timeout: test_timeout, suite: testSuite ) 
	endif
endif

if 'ascend' in enabledBackends and 'hwloc' in enabledBackends
	ascend = executable('ascend', [ 'source/ascend.cpp'], dependencies: [ hicrBuildDep, hicrAscendBackendDep, hicrHWLocBackendDep])

	if get_option('buildTests')
	  test('ascend', ascend, args : [ ], is_parallel: false, timeout: test_timeout, suite: testSuite ) 
	endif
endif
