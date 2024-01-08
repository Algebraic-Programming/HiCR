testSuite = [ 'examples', 'hicr' ]

topologyCPPArgs = []
topologyDependencies = [ hicrBuildDep]

if 'hwloc' in enabledBackends
  topologyCPPArgs += '-D_USE_HWLOC_'
  topologyDependencies += hicrHWLocBackendDep
endif

if 'ascend' in enabledBackends 
	topologyCPPArgs += '-D_USE_ASCEND_'
	topologyDependencies += hicrAscendBackendDep
endif

topology = executable('topology', [ 'source/topology.cpp'], dependencies: topologyDependencies, cpp_args : topologyCPPArgs )

if get_option('buildTests')
	test('topology', topology, args : [ ], suite: testSuite )
endif
