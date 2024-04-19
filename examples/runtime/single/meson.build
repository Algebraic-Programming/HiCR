testSuite = [ 'examples', 'runtime', 'single' ]
test_timeout = 60

if 'pthreads' in enabledBackends and 'hwloc' in enabledBackends
	host = executable('host', [ 'source/main.cpp'], dependencies: hicrBuildDep, include_directories: ['source'], cpp_args: [ '-D_HICR_RUNTIME_DISABLE_AUTODETECT' ] )

	if get_option('buildTests')
	  test('host', host, args : [ meson.current_source_dir() + '/machineModel.json'  ], timeout: test_timeout, suite: testSuite )
	endif
endif

if 'mpi' in enabledBackends and 'pthreads' in enabledBackends and 'hwloc' in enabledBackends
	mpi = executable('mpi', [ 'source/main.cpp'], dependencies: hicrBuildDep, include_directories: ['source'] )

	if get_option('buildTests')
	  test('mpi', mpirunExecutable, args : [ '-n', '1', mpi.full_path(), meson.current_source_dir() + '/machineModel.json'  ], timeout: test_timeout, suite: testSuite )
	endif
endif
