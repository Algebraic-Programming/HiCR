testSuite = [ 'examples', 'objectStore', 'singleInstance' ]
test_timeout = 60

if 'host/hwloc' in enabledBackends
	host = executable('single', [ 'source/main.cpp'], dependencies: hicrBuildDep, include_directories: ['source'] )

	if get_option('buildTests')
	  test('single', host, args : [], timeout: test_timeout, suite: testSuite )
	endif
endif
