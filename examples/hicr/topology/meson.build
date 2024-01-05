testSuite = [ 'examples', 'hicr', 'topology' ]

if 'sharedMemory/hwloc' in enabledBackends
    sharedMemoryDependencies = [ hicrBuildDep, hicrSharedMemoryHWLocBackendDep  ]
	sharedMemory = executable('sharedMemory', [ 'source/sharedMemory.cpp'], dependencies: sharedMemoryDependencies )

	if get_option('buildTests')
	  test('sharedMemory', sharedMemory, args : [ ], suite: testSuite )
	endif
endif
