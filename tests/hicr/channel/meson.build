if 'sequential' in enabledBackends
	test('producerChannel', executable('producerChannel', 'producerChannel.cpp', dependencies: [  testDeps, hicrSequentialBackendDep ] ), protocol: 'gtest', suite: [ 'hicr', 'channel' ] )
	test('consumerChannel', executable('consumerChannel', 'consumerChannel.cpp', dependencies: [  testDeps, hicrSequentialBackendDep ] ), protocol: 'gtest', suite: [ 'hicr', 'channel' ] )
endif

