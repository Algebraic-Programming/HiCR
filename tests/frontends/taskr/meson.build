if 'sharedMemory/hwloc' in enabledBackends and 'sharedMemory/pthreads' in enabledBackends
	test('runtime',  executable('runtime',   'runtime.cpp',  dependencies: [  testDeps, hicrSharedMemoryHWLocBackendDep, hicrSharedMemoryPthreadsBackendDep ] ), protocol: 'gtest', suite: [ 'tests', 'frontends', 'taskr' ] )
endif
