if 'mpi' in enabledBackends and 'host/hwloc' in enabledBackends 
 test('producer', executable('producer', 'producer.cpp', dependencies: HiCRTestBuildDependencies ), protocol: 'gtest', suite: [ 'tests', 'sequential', 'L2', 'channel', 'spsc' ] )
 test('consumer', executable('consumer', 'consumer.cpp', dependencies: HiCRTestBuildDependencies ), protocol: 'gtest', suite: [ 'tests', 'sequential', 'L2', 'channel', 'spsc' ] )
endif

