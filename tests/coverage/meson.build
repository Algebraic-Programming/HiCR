includes = executable('includes',
  files(['includes.cpp']),
  include_directories: hicrIncludes,
  dependencies: [ hicrDeps, testDeps ],
  cpp_args: [ hicrCppArgs, testCppFlags, '-DHICR_ENABLE_BACKEND_SHARED_MEMORY'],
  link_args: [ testLinkFlags ]
  )
  
test('includes', includes,
  protocol: 'gtest',
  suite: 'coverage'
)

