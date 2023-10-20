testSuite = [ 'examples', 'hicr', 'resourceList' ]

if 'sharedMemory' in enabledBackends
	resourceList = executable('resourceList', [ 'source/main.cpp'], dependencies: [ taskrBuildDep, hicrSharedMemoryBackendDep ])
	get_option('buildTests') ? test('resourceList', resourceList, args : [ '5', '0', '1', '2', '3', '4', '5', '6', '7', '8' ], suite: testSuite) :
endif

