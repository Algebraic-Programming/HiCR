testSuite = [ 'examples', 'hicr', 'channel']


exampleBuildIncludes = include_directories([
   'include'
  ])
  
sequential = executable('sequential', [ 'source/sequential.cpp'], dependencies: [ hicrBuildDep, hicrSequentialBackendDep, dependency('threads', required: true)], include_directories: [exampleBuildIncludes] )
test('sequential', sequential, args : [ '2' ], suite: testSuite )
test('sequential', sequential, args : [ '3' ], suite: testSuite )

if 'sharedMemory' in enabledBackends
	sharedMemory = executable('sharedMemory', [ 'source/sharedMemory.cpp'], dependencies: [ hicrBuildDep, hicrSharedMemoryBackendDep], include_directories: [exampleBuildIncludes])
	test('sharedMemory', sharedMemory, args : [ '2' ], suite: testSuite )
	test('sharedMemory', sharedMemory, args : [ '3' ], suite: testSuite )
endif

if 'mpi' in enabledBackends
	mpi = executable('mpi', [ 'source/mpi.cpp'], dependencies: [ hicrBuildDep, hicrMPIBackendDep], include_directories: [exampleBuildIncludes] )
	test('mpi', mpirunExecutable, args : [ '-n', '2', mpi.full_path(), '3' ], suite: testSuite )
endif

if 'lpf' in enabledBackends
	lpf = executable('lpf', [ 'source/lpf.cpp'], dependencies: [ hicrBuildDep, hicrLPFBackendDep], include_directories: [exampleBuildIncludes] )
	test('lpf', lpfrunExecutable, args : [ '-np', '2', '-engine', 'ibverbs', lpf.full_path(), '3' ], suite: testSuite )
endif
