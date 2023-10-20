executable('channel_sequential',
  [ 'source/channel_sequential.cpp'],
  dependencies: [ hicrBuildDep, hicrSequentialBackendDep, dependency('threads', required: true)]
)

if 'mpi' in enabledBackends
	executable('channel_mpi',
	  [ 'source/channel_mpi.cpp'],
	  dependencies: [ hicrBuildDep, hicrMPIBackendDep ]
	)
endif

if 'sharedMemory' in enabledBackends
	executable('channel_sharedMemory',
	  [ 'source/channel_sharedMemory.cpp'],
	  dependencies: [ hicrBuildDep, hicrSharedMemoryBackendDep ]
	)
endif
