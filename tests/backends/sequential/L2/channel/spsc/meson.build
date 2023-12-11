test('producer', executable('producer', 'producer.cpp', dependencies: [  testDeps, hicrSequentialBackendDep ] ), protocol: 'gtest', suite: [ 'tests', 'sequential', 'L2', 'channel', 'spsc' ] )
test('consumer', executable('consumer', 'consumer.cpp', dependencies: [  testDeps, hicrSequentialBackendDep ] ), protocol: 'gtest', suite: [ 'tests', 'sequential', 'L2', 'channel', 'spsc' ] )
