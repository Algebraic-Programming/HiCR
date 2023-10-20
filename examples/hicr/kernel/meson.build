testSuite = [ 'examples', 'hicr', 'kernel' ]

sequential = executable('sequential', [ 'source/sequential.cpp'], dependencies: [ hicrBuildDep, hicrSequentialBackendDep])
get_option('buildTests') ? test('sequential', sequential, args : [ ], suite: testSuite ) :

# if 'ascend' in enabledBackends
# 	ascend = executable('ascend', [ 'source/ascend.cpp'], dependencies: [ hicrBuildDep, hicrAscendBackendDep])
# 	get_option('buildTests') ? test('ascend', ascend, args : [ ], suite: testSuite ) : 
# endif
