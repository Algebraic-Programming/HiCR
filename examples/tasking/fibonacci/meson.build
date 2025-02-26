testSuite = [ 'examples', 'tasking' ]

if 'hwloc' in enabledBackends and 'pthreads' in enabledBackends and 'boost' in enabledBackends
	fibonacci = executable('fibonacci', [ 'source/main.cpp'], dependencies: hicrBuildDep )

	if get_option('buildTests')
	  test('fibonacci', fibonacci, args : [ ], suite: testSuite )
	endif
endif
