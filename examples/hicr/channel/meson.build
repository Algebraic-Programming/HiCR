executable('channel_sequential',
  [ 'source/channel_sequential.cpp'],
  dependencies: [ hicrBuildDep, hicrSequentialBackendDep, dependency('threads', required: true)]
)

executable('channel_mpi',
  [ 'source/channel_mpi.cpp'],
  dependencies: [ hicrBuildDep, hicrMPIBackendDep ]
)

executable('channel_sharedMemory',
  [ 'source/channel_sharedMemory.cpp'],
  dependencies: [ hicrBuildDep, hicrSharedMemoryBackendDep ]
)
