testSuite = [ 'tests', 'frontends', 'taskr' ]
testDependencies = [ testDeps, hicrSharedMemoryHWLocBackendDep, hicrSharedMemoryPthreadsBackendDep ]

if 'sharedMemory/hwloc' in enabledBackends and 'sharedMemory/pthreads' in enabledBackends
	test('runtime',    executable('runtime',    'runtime.cpp',    dependencies: testDependencies ), protocol: 'gtest', suite: testSuite )
	test('task',       executable('task',       'task.cpp',       dependencies: testDependencies ), protocol: 'gtest', suite: testSuite )
    test('worker',     executable('worker',     'worker.cpp',     dependencies: testDependencies ), protocol: 'gtest', suite: testSuite )
    test('dispatcher', executable('dispatcher', 'dispatcher.cpp', dependencies: testDependencies ), protocol: 'gtest', suite: testSuite )
endif
