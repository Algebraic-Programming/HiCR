unit_pthreads = executable('unit_pthreads',
  files(['pthreads.cpp']),
  dependencies: [ testDeps, hicrPthreadsBackendDep]
  )
  
test('pthreads', unit_pthreads,
  protocol: 'gtest',
  suite: [ 'backends', 'sharedMemory', 'pthreads', 'unit' ]
)

