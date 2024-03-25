testSuite = [ 'examples', 'tasking' ]

if 'hwloc' in enabledBackends and 'pthreads' in enabledBackends
	conditionVariable = executable('conditionVariableExample', [ 'source/main.cpp'], dependencies: hicrBuildDep )

	if get_option('buildTests')
	  test('conditionVariable', conditionVariable, args : [ ], suite: testSuite )
	endif
endif
