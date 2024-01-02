testSuite = [ 'examples', 'hicr', 'channels', 'mpsc']
test_timeout = 180

exampleBuildIncludes = include_directories([
   'source/include'
  ])
  
if 'sharedMemory' in enabledBackends
	sharedMemory = executable('sharedMemory', [ 'source/sharedMemory.cpp'], dependencies: [ hicrBuildDep, hicrSharedMemoryBackendDep], include_directories: [exampleBuildIncludes])

	if get_option('buildTests')
	  test('sharedMemory', sharedMemory, args : [ '3', '15' ], timeout: test_timeout, suite: testSuite )
	endif
endif

if 'mpi' in enabledBackends
	mpi = executable('mpi', [ 'source/mpi.cpp'], dependencies: [ hicrBuildDep, hicrMPIBackendDep], include_directories: [exampleBuildIncludes] )

	if get_option('buildTests')
	test('mpi', mpirunExecutable, args : [ '-n', '16', mpi.full_path(), '3' ], timeout: test_timeout, suite: testSuite )
	endif
endif