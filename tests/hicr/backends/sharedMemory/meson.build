# Registering tests
test('coverage',     executable('_coverage',    'coverage.cpp',     dependencies: [  testDeps, hicrPthreadsBackendDep ] ), protocol: 'gtest', suite: [ 'hicr', 'backends' ] )
test('sharedMemory', executable('sharedMemory', 'sharedMemory.cpp', dependencies: [  testDeps, hicrPthreadsBackendDep ] ), protocol: 'gtest', suite: [ 'hicr', 'backends' ] )

