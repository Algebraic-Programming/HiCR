if 'hwloc' in enabledBackends and 'pthreads' in enabledBackends and 'boost' in enabledBackends
    test('processingUnit', executable('processingUnit', 'processingUnit.cpp', dependencies: HiCRTestBuildDependencies ), protocol: 'gtest', suite: [ 'tests', 'backends', 'pthreads' ] )
    test('coroutine', executable('coroutine', 'coroutine.cpp', dependencies: HiCRTestBuildDependencies ), protocol: 'gtest', suite: [ 'tests', 'backends', 'pthreads' ] )
endif


