testSuite = [ 'tests', 'frontends', 'taskr' ]

if 'sharedMemory/hwloc' in enabledBackends and 'sharedMemory/pthreads' in enabledBackends
    testDependencies = [ testDeps, hicrSharedMemoryHWLocBackendDep, hicrSharedMemoryPthreadsBackendDep ]
	test('runtime',    executable('runtime',    'runtime.cpp',    dependencies: testDependencies ), protocol: 'gtest', suite: testSuite )
	test('task',       executable('task',       'task.cpp',       dependencies: testDependencies ), protocol: 'gtest', suite: testSuite )
    test('worker',     executable('worker',     'worker.cpp',     dependencies: testDependencies ), protocol: 'gtest', suite: testSuite )
    test('dispatcher', executable('dispatcher', 'dispatcher.cpp', dependencies: testDependencies ), protocol: 'gtest', suite: testSuite )
endif
