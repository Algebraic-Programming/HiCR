testSuite = [ 'examples', 'hicr', 'kernel' ]

sequential = executable('sequential', [ 'source/sequential.cpp'], dependencies: [ hicrBuildDep, hicrSequentialBackendDep])
test('sequential', sequential, args : [ ], suite: testSuite )

if 'ascend' in enabledBackends
	ascend = executable('ascend', [ 'source/ascend.cpp'], cpp_args:['-g'], dependencies: [ hicrBuildDep, hicrAscendBackendDep])
	test('ascend', ascend, args : [ ], suite: testSuite )
endif
