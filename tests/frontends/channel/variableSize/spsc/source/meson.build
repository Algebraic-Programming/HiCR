testSuite = ['tests', 'channels', 'variableSize', 'spsc', 'distributed']
test_timeout = 60

sourceList = [
	'fillBufferOneToken',
	'fillBufferMultipleTokens',
	'fillCoordination',
	'excessBuffer',
]

foreach source : sourceList
	if 'mpi' in enabledBackends and 'hwloc' in enabledBackends
		mpi = executable(
			source,
			[source + '.cpp'],
			dependencies: channelsTestMPIDep,
			include_directories: [exampleBuildIncludes],
		)

		if get_option('buildTests')
			test(
				source,
				mpirunExecutable,
				args: ['-np', '2', '--oversubscribe', mpi.full_path()],
				timeout: test_timeout,
				suite: testSuite,
			)
		endif
	endif
endforeach