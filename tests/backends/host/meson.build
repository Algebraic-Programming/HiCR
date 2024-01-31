if 'hwloc' in enabledBackends
  subdir('hwloc')
endif

if 'pthreads' in enabledBackends
  subdir('pthreads')
endif

test('coroutine', executable('coroutine', 'coroutine.cpp', dependencies: HiCRTestBuildDependencies), protocol: 'gtest', suite: [ 'tests', 'backends', 'sharedMemory', 'coroutine' ] )
