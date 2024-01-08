testSuite = [ 'examples', 'hicr', 'kernel' ]

if 'sharedMemory/hwloc' in enabledBackends and 'sharedMemory/pthreads' in enabledBackends
    sharedMemoryDependencies = [ hicrBuildDep, hicrSharedMemoryHWLocBackendDep, hicrSharedMemoryPthreadsBackendDep] 
	sharedMemory = executable('sharedMemory', [ 'source/sharedMemory.cpp' ], dependencies: sharedMemoryDependencies)

	if get_option('buildTests')
	 test('sharedMemory', sharedMemory, suite: testSuite )
	endif
endif

if 'ascend' in enabledBackends and 'sharedMemory/hwloc' in enabledBackends
	ascend = executable('ascend', [ 'source/ascend.cpp'], dependencies: [ hicrBuildDep, hicrAscendBackendDep, hicrSharedMemoryHWLocBackendDep])

	if get_option('buildTests')
	  test('ascend', ascend, args : [ ], is_parallel: false, suite: testSuite )
	endif
endif
