subdir('unit')

coverage = executable('_coverage',
  files(['coverage.cpp']),
  dependencies: [  testDeps, hicrPthreadsBackendDep  ],
  )
  
test('coverage', coverage,
  protocol: 'gtest',
  suite: [ 'backends', 'sharedMemory', 'pthreads' ]
)

