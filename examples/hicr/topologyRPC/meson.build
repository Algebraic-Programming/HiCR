testSuite = [ 'examples', 'hicr', 'topoogyRPC' ]
test_timeout = 20

topologyDependencies = [ hicrHWLocBackendDep ] 

if 'ascend' in enabledBackends 
	topologyDependencies += hicrAscendBackendDep
endif

if 'mpi' in enabledBackends and 'pthreads' in enabledBackends and 'hwloc' in enabledBackends
	mpi = executable('mpi', [ 'source/mpi.cpp'], dependencies: [ hicrBuildDep, hicrMPIBackendDep, hicrPthreadsBackendDep, topologyDependencies ], include_directories: ['source'] )

	if get_option('buildTests')
	  test('mpi', mpirunExecutable, args : [ '-n', '4', mpi.full_path() ], timeout: test_timeout, suite: testSuite )
	endif
endif
