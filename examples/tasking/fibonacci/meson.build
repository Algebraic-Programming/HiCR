testSuite = [ 'examples', 'tasking' ]

if 'host/hwloc' in enabledBackends and 'host/pthreads' in enabledBackends
	fibonacci = executable('fibonacci', [ 'source/main.cpp'], dependencies: hicrBuildDep )

	if get_option('buildTests')
	  test('fibonacci', fibonacci, args : [ ], suite: testSuite )
	endif
endif
