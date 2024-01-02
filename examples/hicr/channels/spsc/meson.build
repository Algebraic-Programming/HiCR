testSuite = [ 'examples', 'hicr', 'channel', 'spsc']
test_timeout = 180

exampleBuildIncludes = include_directories([
   'source/include'
  ])
  
sequential = executable('sequential', [ 'source/sequential.cpp'], dependencies: [ hicrBuildDep, hicrSequentialBackendDep, dependency('threads', required: true)], include_directories: [exampleBuildIncludes] )

if get_option('buildTests')
  test('sequential', sequential, args : [ '2' ], timeout: test_timeout, suite: testSuite )
  test('sequential', sequential, args : [ '3' ], timeout: test_timeout, suite: testSuite )
endif

if 'sharedMemory' in enabledBackends
	sharedMemory = executable('sharedMemory', [ 'source/sharedMemory.cpp'], dependencies: [ hicrBuildDep, hicrSharedMemoryBackendDep], include_directories: [exampleBuildIncludes])

	if get_option('buildTests')
	 test('sharedMemory', sharedMemory, args : [ '2' ], timeout: test_timeout, suite: testSuite )
	 test('sharedMemory', sharedMemory, args : [ '3' ], timeout: test_timeout, suite: testSuite )
	endif
endif

if 'mpi' in enabledBackends
	mpi = executable('mpi', [ 'source/mpi.cpp'], dependencies: [ hicrBuildDep, hicrMPIBackendDep], include_directories: [exampleBuildIncludes] )

	if get_option('buildTests')
	  test('mpi', mpirunExecutable, args : [ '-n', '2', mpi.full_path(), '3' ], timeout: test_timeout, suite: testSuite )
    endif
endif

if 'lpf' in enabledBackends
	lpf = executable('lpf', [ 'source/lpf.cpp'], dependencies: [ hicrBuildDep, hicrLPFBackendDep], include_directories: [exampleBuildIncludes] )

	if get_option('buildTests')
	  test('lpf', lpfrunExecutable, args : [ '-np', '2', '-engine', 'ibverbs', lpf.full_path(), '3' ], timeout: test_timeout, suite: testSuite )
	endif
endif
