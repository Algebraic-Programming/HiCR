testSuite = ['examples', 'objectStore', 'publishRead']
test_timeout = 60

if 'lpf' in enabledBackends and 'host/hwloc' in enabledBackends
	lpf = executable('lpf', [ 'source/lpf.cpp' ], dependencies: hicrBuildDep, include_directories: [exampleBuildIncludes] )

	if get_option('buildTests')
	  test('lpf', lpfrunExecutable, args : [ '-np', '2', '-engine', 'zero', lpf.full_path() ], timeout: test_timeout, suite: testSuite )
	endif
endif
