testDeps = [ hicrBuildDep, hicrTestDep ]

subdir('unit')

coverage = executable('_coverage',
  files(['coverage.cpp']),
  dependencies: [  testDeps ],
  )
  
test('coverage', coverage,
  protocol: 'gtest',
  suite: 'hicr'
)

