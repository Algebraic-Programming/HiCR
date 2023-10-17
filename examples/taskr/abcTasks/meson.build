executable('abcTasks_sequential',
  [ 'source/main_sequential.cpp'],
  dependencies: [ taskrBuildDep, hicrSequentialBackendDep ]
)

if 'sharedMemory' in enabledBackends
	executable('abcTasks_sharedMemory',
	  [ 'source/main_sharedMemory.cpp'],
	  dependencies: [ taskrBuildDep, hicrSharedMemoryBackendDep]
	)
endif
