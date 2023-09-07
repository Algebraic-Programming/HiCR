# Registering Task class tests
coroutine = executable('couroutine',
  files(['coroutine.cpp']),
  dependencies: [ testDeps ],
  )
  
test('coroutine', coroutine,
  protocol: 'gtest',
  suite: [ 'hicr', 'common' ]
)

