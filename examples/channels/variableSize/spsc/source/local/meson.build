testSuite = ['examples', 'channels', 'variableSize', 'spsc', 'local']
test_timeout = 60

if 'pthreads' in enabledBackends and 'hwloc' in enabledBackends
	pthreads = executable(
		'pthreads',
		['pthreads.cpp'],
		dependencies: hicrBuildDep,
		include_directories: [exampleBuildIncludes],
	)

	if get_option('buildTests')
		test('pthreads', pthreads, args: ['3'], timeout: test_timeout, suite: testSuite)
	endif
endif