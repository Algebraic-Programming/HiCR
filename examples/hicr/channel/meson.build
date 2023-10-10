#dependency('mpi', language : 'cpp')
thread_dep=dependency('threads',required : true)
#executable('channel_sequential',
#  [ 'source/channel_sequential.cpp'],
#  dependencies: [ hicrBuildDep, hicrSequentialBackendDep, thread_dep]
#)

#executable('channel_mpi',
#  [ 'source/channel_mpi.cpp'],
#  dependencies: [ hicrBuildDep]
#)

executable('channel_lpf',
  [ 'source/channel_lpf.cpp'],
  dependencies: [ hicrBuildDep]
)

#executable('channel_sharedMemory',
#  [ 'source/channel_sharedMemory.cpp'],
#  dependencies: [ hicrBuildDep, hicrSharedMemoryBackendDep, thread_dep]
#)
