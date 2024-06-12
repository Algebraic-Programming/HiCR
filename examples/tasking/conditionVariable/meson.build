testSuite = [ 'examples', 'tasking' ]

if 'host/hwloc' in enabledBackends and 'host/pthreads' in enabledBackends
	conditionVariable = executable('conditionVariableExample', [ 'source/main.cpp'], dependencies: hicrBuildDep )

	if get_option('buildTests')
	  test('conditionVariable', conditionVariable, args : [ ], suite: testSuite )
	endif
endif
