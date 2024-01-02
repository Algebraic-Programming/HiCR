testSuite = [ 'examples', 'hicr', 'kernel' ]

sequential = executable('sequential', [ 'source/sequential.cpp'], dependencies: [ hicrBuildDep, hicrSequentialBackendDep])

if get_option('buildTests')
  test('sequential', sequential, args : [ ], suite: testSuite )
endif

if 'ascend' in enabledBackends
	ascend = executable('ascend', [ 'source/ascend.cpp'], dependencies: [ hicrBuildDep, hicrAscendBackendDep])

	if get_option('buildTests')
	  test('ascend', ascend, args : [ ], is_parallel: false, suite: testSuite )
	endif
endif
