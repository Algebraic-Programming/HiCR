testSuite = [ 'tests', 'frontends', 'tasking' ]

if 'host/hwloc' in enabledBackends and 'host/pthreads' in enabledBackends
    test('task',       executable('task',       'task.cpp',           dependencies: HiCRTestBuildDependencies ), protocol: 'gtest', suite: testSuite )
    test('worker',     executable('worker',     'worker.cpp',         dependencies: HiCRTestBuildDependencies ), protocol: 'gtest', suite: testSuite )
    test('mutexTest',  executable('mutexTest',  'mutexTest.cpp',      dependencies: HiCRTestBuildDependencies ), protocol: 'gtest', suite: testSuite )
endif
