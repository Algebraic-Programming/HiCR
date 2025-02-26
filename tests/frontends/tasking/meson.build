testSuite = [ 'tests', 'frontends', 'tasking' ]

if 'hwloc' in enabledBackends and 'pthreads' in enabledBackends and 'boost' in enabledBackends
    test('task',       executable('task',       'task.cpp',           dependencies: HiCRTestBuildDependencies ), protocol: 'gtest', suite: testSuite )
    test('worker',     executable('worker',     'worker.cpp',         dependencies: HiCRTestBuildDependencies ), protocol: 'gtest', suite: testSuite )
endif
