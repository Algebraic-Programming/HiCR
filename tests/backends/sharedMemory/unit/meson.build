sharedMemory = executable('sharedMemory',
  files(['sharedMemory.cpp']),
  dependencies: [ testDeps, hicrPthreadsBackendDep]
  )
  
test('sharedMemory', sharedMemory,
  protocol: 'gtest',
  suite: [ 'backends', 'sharedMemory', 'unit' ]
)

