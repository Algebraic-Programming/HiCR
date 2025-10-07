testSuite = ['tests', 'channels', 'variableSize', 'spsc', 'distributed']
test_timeout = 60

if 'mpi' in enabledBackends and 'hwloc' in enabledBackends
	mpi = executable(
		'fillBuffers',
		['fillBuffers.cpp'],
		dependencies: channelsTestMPIDep,
		include_directories: [exampleBuildIncludes],
	)

	if get_option('buildTests')
		test(
			'fillBuffers',
			mpirunExecutable,
			args: ['-np', '2', '--oversubscribe', mpi.full_path()],
			timeout: test_timeout,
			suite: testSuite,
		)
	endif
endif