if 'hwloc' in enabledBackends and 'pthreads' in enabledBackends and 'boost' in enabledBackends

subdir('L0')
test('coroutine', executable('coroutine', 'coroutine.cpp', dependencies: HiCRTestBuildDependencies), protocol: 'gtest', suite: [ 'tests', 'backends', 'boost', 'coroutine' ] )

endif


