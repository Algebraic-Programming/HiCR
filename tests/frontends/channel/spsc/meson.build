if 'mpi' in enabledBackends and 'sharedMemory/hwloc' in enabledBackends 
 test('producer', executable('producer', 'producer.cpp', dependencies: [  testDeps, hicrSharedMemoryHWLocBackendDep ] ), protocol: 'gtest', suite: [ 'tests', 'sequential', 'L2', 'channel', 'spsc' ] )
 test('consumer', executable('consumer', 'consumer.cpp', dependencies: [  testDeps, hicrSharedMemoryHWLocBackendDep ] ), protocol: 'gtest', suite: [ 'tests', 'sequential', 'L2', 'channel', 'spsc' ] )
endif

