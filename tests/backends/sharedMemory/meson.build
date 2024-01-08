if 'sharedMemory/hwloc' in enabledBackends
  subdir('hwloc')
endif

if 'sharedMemory/pthreads' in enabledBackends
  subdir('pthreads')
endif

test('coroutine', executable('coroutine', 'coroutine.cpp', dependencies: [  testDeps, dependency('boost',  modules : ['context'],  required : true) ] ), protocol: 'gtest', suite: [ 'tests', 'backends', 'sharedMemory', 'coroutine' ] )
