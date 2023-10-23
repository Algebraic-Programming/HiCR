testSuite = [ 'examples', 'hicr', 'channel' ]

sequential = executable('sequential', [ 'source/sequential.cpp'], dependencies: [ hicrBuildDep, hicrSequentialBackendDep, dependency('threads', required: true)])
test('sequential', sequential, args : [ '2' ], suite: testSuite )
test('sequential', sequential, args : [ '3' ], suite: testSuite )

if 'sharedMemory' in enabledBackends
	sharedMemory = executable('sharedMemory', [ 'source/sharedMemory.cpp'], dependencies: [ hicrBuildDep, hicrSharedMemoryBackendDep ])
	test('sharedMemory', sharedMemory, args : [ '2' ], suite: testSuite )
	test('sharedMemory', sharedMemory, args : [ '3' ], suite: testSuite )
endif

if 'mpi' in enabledBackends
	mpi = executable('mpi', [ 'source/mpi.cpp'], dependencies: [ hicrBuildDep, hicrMPIBackendDep ] )
	#test('mpi', mpi, args : [ '3' ], suite: testSuite )
endif
