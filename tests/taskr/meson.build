if 'sharedMemory' in enabledBackends
	test('runtime',  executable('runtime',   'runtime.cpp',  dependencies: [  testDeps, hicrSharedMemoryBackendDep ] ), protocol: 'gtest', suite: [ 'taskr' ] )
endif
