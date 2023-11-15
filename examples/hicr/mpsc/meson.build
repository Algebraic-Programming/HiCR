testSuite = [ 'examples', 'hicr', 'mpsc']
test_timeout = 180

exampleBuildIncludes = include_directories([
   'source/include'
  ])
  
if 'sharedMemory' in enabledBackends
	sharedMemory = executable('sharedMemory', [ 'source/sharedMemory.cpp'], dependencies: [ hicrBuildDep, hicrSharedMemoryBackendDep], include_directories: [exampleBuildIncludes])
	test('sharedMemory', sharedMemory, args : [ '2' ], timeout: test_timeout, suite: testSuite )
	test('sharedMemory', sharedMemory, args : [ '3' ], timeout: test_timeout, suite: testSuite )
endif

if 'mpi' in enabledBackends
	mpi = executable('mpi', [ 'source/mpi.cpp'], dependencies: [ hicrBuildDep, hicrMPIBackendDep], include_directories: [exampleBuildIncludes] )
	test('mpi', mpirunExecutable, args : [ '-n', '2', mpi.full_path(), '3' ], timeout: test_timeout, suite: testSuite )
endif