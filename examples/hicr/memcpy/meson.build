testSuite = [ 'examples', 'hicr', 'memcpy' ]

if 'sharedMemory/hwloc' in enabledBackends and 'sharedMemory/pthreads' in enabledBackends
    sharedMemoryDependencies = [ hicrBuildDep, hicrSharedMemoryHWLocBackendDep, hicrSharedMemoryPthreadsBackendDep ]
	sharedMemory = executable('sharedMemory', [ 'source/sharedMemory.cpp'], dependencies: sharedMemoryDependencies )

	if get_option('buildTests')
	  test('sharedMemory', sharedMemory, args : [ ], suite: testSuite )
	endif
endif

if 'lpf' in enabledBackends and 'sharedMemory/hwloc' in enabledBackends 
	lpf = executable('lpf', [ 'source/lpf.cpp'], dependencies: [ hicrBuildDep, hicrLPFBackendDep, hicrSharedMemoryHWLocBackendDep])

	if get_option('buildTests')
	  test('lpf', lpfrunExecutable, args : [ '-np', '2', '-engine', 'ibverbs', lpf.full_path() ], suite: testSuite ) 
	endif
endif


if 'ascend' in enabledBackends and 'sharedMemory/hwloc' in enabledBackends
	ascend = executable('ascend', [ 'source/ascend.cpp'], dependencies: [ hicrBuildDep, hicrAscendBackendDep, hicrSharedMemoryHWLocBackendDep])

	if get_option('buildTests')
	  test('ascend', ascend, args : [ ], is_parallel: false, suite: testSuite ) 
	endif
endif
