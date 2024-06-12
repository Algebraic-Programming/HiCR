testSuite = [ 'examples', 'deployer', 'multiple' ]
test_timeout = 60

if 'mpi' in enabledBackends and 'pthreads' in enabledBackends and 'hwloc' in enabledBackends
	mpi = executable('mpi', [ 'source/main.cpp'], dependencies: hicrBuildDep, include_directories: ['source'] )

	if get_option('buildTests')
	  test('mpi', mpirunExecutable, args : [ '-n', '7', mpi.full_path(), meson.current_source_dir() + '/machineModel.json'  ], timeout: test_timeout, suite: testSuite )
	endif
endif
