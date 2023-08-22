unit_task = executable('unit_task',
  files(['task.cpp']),
  include_directories: hicrIncludes,
  dependencies: [ hicrDeps, testDeps ],
  cpp_args: [ hicrCppArgs, testCppFlags, '-DHICR_ENABLE_BACKEND_SHARED_MEMORY'],
  link_args: [ testLinkFlags ]
  )
  
test('task', unit_task,
  protocol: 'gtest',
  suite: 'unit'
)

