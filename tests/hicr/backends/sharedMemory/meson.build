if 'sharedMemory' in enabledBackends
	test('sharedMemory', executable('sharedMemory', 'sharedMemory.cpp', dependencies: [  testDeps, hicrSharedMemoryBackendDep ] ), protocol: 'gtest', suite: [ 'hicr', 'backends', 'sharedMemory' ] )
	test('thread', executable('thread', 'thread.cpp', dependencies: [  testDeps, hicrSharedMemoryBackendDep ] ), protocol: 'gtest', suite: [ 'hicr', 'backends', 'sharedMemory' ] )
endif

