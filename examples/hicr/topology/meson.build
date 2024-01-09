testSuite = [ 'examples', 'hicr' ]

topologyDependencies = [ ]

if 'hwloc' in enabledBackends
  topologyDependencies += hicrHWLocBackendDep
endif

if 'ascend' in enabledBackends 
	topologyDependencies += hicrAscendBackendDep
endif

topology = executable('topology', [ 'source/topology.cpp'], dependencies: [ hicrBuildDep, topologyDependencies ] )

if get_option('buildTests')
	test('topology', topology, args : [ ], suite: testSuite )
endif
