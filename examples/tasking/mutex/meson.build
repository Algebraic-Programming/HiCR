testSuite = [ 'examples', 'tasking' ]

if 'host/hwloc' in enabledBackends and 'host/pthreads' in enabledBackends
	mutex = executable('mutexExample', [ 'source/main.cpp'], dependencies: hicrBuildDep )

	if get_option('buildTests')
	  test('mutex', mutex, args : [ ], suite: testSuite )
	endif
endif
