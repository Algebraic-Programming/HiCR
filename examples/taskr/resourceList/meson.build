if 'sharedMemory' in enabledBackends
	executable('resourceList',
	  [ 'source/main.cpp'],
	  dependencies: [ taskrBuildDep, hicrSharedMemoryBackendDep],
	  include_directories: include_directories(['source'])
	)
endif