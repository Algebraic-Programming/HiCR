testSuite = [ 'tests', 'frontends', 'taskr' ]

if 'hwloc' in enabledBackends and 'pthreads' in enabledBackends
    testDependencies = [ testDeps, hicrHWLocBackendDep, hicrPthreadsBackendDep ]
	test('runtime',    executable('runtime',    'runtime.cpp',    dependencies: testDependencies ), protocol: 'gtest', suite: testSuite )
	test('task',       executable('task',       'task.cpp',       dependencies: testDependencies ), protocol: 'gtest', suite: testSuite )
    test('worker',     executable('worker',     'worker.cpp',     dependencies: testDependencies ), protocol: 'gtest', suite: testSuite )
    test('dispatcher', executable('dispatcher', 'dispatcher.cpp', dependencies: testDependencies ), protocol: 'gtest', suite: testSuite )
endif
