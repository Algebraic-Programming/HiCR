executable('kernelSequential',
  [ 'source/kernelSequential.cpp'],
  dependencies: [ hicrBuildDep, hicrSequentialBackendDep]
)

if 'ascend' in enabledBackends
	executable('kernelAscend',
	  [ 'source/kernelAscend.cpp'],
	  dependencies: [ hicrBuildDep, hicrAscendBackendDep]
	)
endif
