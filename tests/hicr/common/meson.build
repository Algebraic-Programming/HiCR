# Registering Task class tests
coroutine = executable('couroutine',
  files(['coroutine.cpp']),
  dependencies: [ testDeps, dependency('threads', required: true) ],
  )
  
test('coroutine', coroutine,
  protocol: 'gtest',
  suite: [ 'hicr', 'common' ]
)

