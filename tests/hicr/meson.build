testDeps = [ hicrBuildDep, hicrTestDep ]

# Registering backend tests
subdir('backends')

# Registering common tests
subdir('common')

# Registering tests
test('coverage', executable('_coverage', 'coverage.cpp', dependencies: [  testDeps ] ), protocol: 'gtest', suite: [ 'hicr' ] )
test('task',     executable('task',      'task.cpp',     dependencies: [  testDeps ] ), protocol: 'gtest', suite: [ 'hicr' ] )
test('worker',   executable('worker',    'worker.cpp',   dependencies: [  testDeps ] ), protocol: 'gtest', suite: [ 'hicr' ] )

