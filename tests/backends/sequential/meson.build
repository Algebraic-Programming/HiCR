subdir('L0')
subdir('L1')

testSuite = [ 'tests', 'backends', 'sequential']
testDependencies = [ testDeps, hicrSequentialBackendDep ]
test('coroutine',    executable('coroutine',    'coroutine.cpp',    dependencies: testDependencies ), protocol: 'gtest', suite: testSuite )
