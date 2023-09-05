runtime = executable('runtime',
  files(['runtime.cpp']),
  dependencies: [ testDeps, hicrPthreadsBackendDep]
  )
  
test('runtime', runtime,
  protocol: 'gtest',
  suite: [ 'frontends', 'taskr', 'unit' ]
)

