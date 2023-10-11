if 'sequential' in enabledBackends
	executable('kernelSequential',
	  [ 'source/kernelSequential.cpp'],
	  dependencies: [ hicrBuildDep, hicrSequentialBackendDep]
	)
endif

if 'ascend' in enabledBackends
	executable('kernelAscend',
	  [ 'source/kernelAscend.cpp'],
	  dependencies: [ hicrBuildDep, hicrAscendBackendDep]
	)
endif
