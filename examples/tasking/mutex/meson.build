testSuite = [ 'examples', 'tasking' ]

if 'hwloc' in enabledBackends and 'pthreads' in enabledBackends
	mutexExample = executable('mutexExample', [ 'source/main.cpp'], dependencies: hicrBuildDep )

	if get_option('buildTests')
	  test('mutex', mutexExample, args : [ ], suite: testSuite )
	endif
endif
