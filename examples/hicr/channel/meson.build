testSuite = [ 'examples', 'hicr', 'channel' ]

sequential = executable('sequential', [ 'source/sequential.cpp'], dependencies: [ hicrBuildDep, hicrSequentialBackendDep, dependency('threads', required: true)])
get_option('buildTests') ? test('sequential', sequential, args : [ '2' ], suite: testSuite ) :
get_option('buildTests') ? test('sequential', sequential, args : [ '3' ], suite: testSuite ) :

if 'sharedMemory' in enabledBackends
	sharedMemory = executable('sharedMemory', [ 'source/sharedMemory.cpp'], dependencies: [ hicrBuildDep, hicrSharedMemoryBackendDep ])
	get_option('buildTests') ? test('sharedMemory', sharedMemory, args : [ '2' ], suite: testSuite ) :
	get_option('buildTests') ? test('sharedMemory', sharedMemory, args : [ '3' ], suite: testSuite ) :
endif

if 'mpi' in enabledBackends
	mpi = executable('mpi', [ 'source/mpi.cpp'], dependencies: [ hicrBuildDep, hicrMPIBackendDep ] )
	#get_option('buildTests') ? test('mpi', mpi, args : [ '3' ], suite: testSuite ) :
endif

