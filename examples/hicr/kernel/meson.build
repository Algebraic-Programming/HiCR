testSuite = [ 'examples', 'hicr', 'kernel' ]

sequential = executable('sequential', [ 'source/sequential.cpp'], dependencies: [ hicrBuildDep, hicrSequentialBackendDep])
test('sequential', sequential, args : [ ], suite: testSuite )

if 'ascend' in enabledBackends
	ascend = executable('ascend', [ 'source/ascend.cpp'], dependencies: [ hicrBuildDep, hicrAscendBackendDep])
	test('ascend', ascend, args : [ ], is_parallel: false, suite: testSuite )
endif
