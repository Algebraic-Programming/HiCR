test('producer', executable('producer', 'producer.cpp', dependencies: [  testDeps, hicrSequentialBackendDep ] ), protocol: 'gtest', suite: [ 'hicr', 'channel', 'spsc' ] )
test('consumer', executable('consumer', 'consumer.cpp', dependencies: [  testDeps, hicrSequentialBackendDep ] ), protocol: 'gtest', suite: [ 'hicr', 'channel', 'spsc' ] )
