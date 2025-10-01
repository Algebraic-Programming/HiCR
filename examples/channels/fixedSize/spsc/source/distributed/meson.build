testSuite = ['examples', 'channels', 'fixedSize', 'spsc', 'distributed']
test_timeout = 60

if 'mpi' in enabledBackends and 'hwloc' in enabledBackends
	mpi = executable(
		'mpi',
		['mpi.cpp'],
		dependencies: hicrBuildDep,
		include_directories: [exampleBuildIncludes],
	)

	if get_option('buildTests')
		test(
			'mpi',
			mpirunExecutable,
			args: ['-n', '2', '--oversubscribe', mpi.full_path(), '3'],
			timeout: test_timeout,
			suite: testSuite,
		)
	endif
endif

if 'lpf' in enabledBackends and 'hwloc' in enabledBackends
	lpf = executable(
		'lpf',
		['lpf.cpp'],
		dependencies: hicrBuildDep,
		include_directories: [exampleBuildIncludes],
	)

	if get_option('buildTests')
		test(
			'lpf',
			lpfrunExecutable,
			args: ['-np', '2', '-engine', 'zero', lpf.full_path(), '3'],
			timeout: test_timeout,
			suite: testSuite,
		)
	endif
endif