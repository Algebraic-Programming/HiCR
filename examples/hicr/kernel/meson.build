testSuite = [ 'examples', 'hicr', 'kernel' ]

if 'hwloc' in enabledBackends and 'pthreads' in enabledBackends
    sharedMemoryDependencies = [ hicrBuildDep, hicrHWLocBackendDep, hicrPthreadsBackendDep] 
	sharedMemory = executable('sharedMemory', [ 'source/sharedMemory.cpp' ], dependencies: sharedMemoryDependencies)

	if get_option('buildTests')
	 test('sharedMemory', sharedMemory, suite: testSuite )
	endif
endif

if 'ascend' in enabledBackends and 'hwloc' in enabledBackends
	ascend = executable('ascend', [ 'source/ascend.cpp'], dependencies: [ hicrBuildDep, hicrAscendBackendDep, hicrHWLocBackendDep])

	if get_option('buildTests')
	  test('ascend', ascend, args : [ ], is_parallel: false, suite: testSuite )
	endif
endif
