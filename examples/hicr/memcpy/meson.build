executable('memcpySequential',
  [ 'source/memcpySequential.cpp'],
  dependencies: [ hicrBuildDep, hicrSequentialBackendDep]
)


if 'sharedMemory' in enabledBackends
	executable('memcpySharedMem',
	  [ 'source/memcpySharedMem.cpp'],
	  dependencies: [ hicrBuildDep, hicrSharedMemoryBackendDep]
	)
endif

if 'ascend' in enabledBackends 
	executable('memcpyAscend',
	  [ 'source/memcpyAscend.cpp'],
	  dependencies: [ hicrBuildDep, hicrAscendBackendDep]
	)
endif
