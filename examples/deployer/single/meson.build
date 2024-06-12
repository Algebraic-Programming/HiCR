testSuite = [ 'examples', 'deployer', 'single' ]
test_timeout = 60

if 'host/pthreads' in enabledBackends and 'host/hwloc' in enabledBackends
	host = executable('host', [ 'source/main.cpp'], dependencies: hicrBuildDep, include_directories: ['source'], cpp_args: [ '-D_HICR_DEPLOYER_DISABLE_AUTODETECT' ] )

	if get_option('buildTests')
	  test('host', host, args : [ meson.current_source_dir() + '/machineModel.json'  ], timeout: test_timeout, suite: testSuite )
	endif
endif

if 'mpi' in enabledBackends and 'host/pthreads' in enabledBackends and 'host/hwloc' in enabledBackends
	mpi = executable('mpi', [ 'source/main.cpp'], dependencies: hicrBuildDep, include_directories: ['source'] )

	if get_option('buildTests')
	  test('mpi', mpirunExecutable, args : [ '-n', '1', mpi.full_path(), meson.current_source_dir() + '/machineModel.json'  ], timeout: test_timeout, suite: testSuite )
	endif
endif
