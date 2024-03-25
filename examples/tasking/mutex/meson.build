testSuite = [ 'examples', 'tasking' ]

if 'hwloc' in enabledBackends and 'pthreads' in enabledBackends
	mutex = executable('mutexExample', [ 'source/main.cpp'], dependencies: hicrBuildDep )

	if get_option('buildTests')
	  test('mutex', mutex, args : [ ], suite: testSuite )
	endif
endif
