unit_task = executable('unit_task',
  files(['task.cpp']),
  dependencies: [ testDeps ],
  )
  
test('task', unit_task,
  protocol: 'gtest',
  suite: 'unit'
)

