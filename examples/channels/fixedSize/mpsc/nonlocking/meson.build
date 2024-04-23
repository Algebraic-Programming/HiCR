testSuite = [ 'examples', 'channels', 'fixedSize', 'mpsc', 'nonlocking']
test_timeout = 60

exampleBuildIncludes = include_directories([
   'include'
  ])
  
if 'hwloc' in enabledBackends and 'pthreads' in enabledBackends
	sharedMemory = executable('sharedMemory', [ 'source/sharedMemory.cpp' ], dependencies: hicrBuildDep, include_directories: [exampleBuildIncludes])

	if get_option('buildTests')
	  test('sharedMemory', sharedMemory, args : [ '3', '15' ], timeout: test_timeout, suite: testSuite )
	endif
endif

if 'mpi' in enabledBackends and 'hwloc' in enabledBackends 
	mpi = executable('mpi', [ 'source/mpi.cpp' ], dependencies: hicrBuildDep, include_directories: [exampleBuildIncludes] )

	if get_option('buildTests')
	test('mpi', mpirunExecutable, args : [ '-n', '16', mpi.full_path(), '3' ], timeout: test_timeout, suite: testSuite )
	endif
endif

if 'lpf' in enabledBackends and 'hwloc' in enabledBackends 
	lpf = executable('lpf', [ 'source/lpf.cpp' ], dependencies: hicrBuildDep, include_directories: [exampleBuildIncludes] )
	test('lpf', lpfrunExecutable, args : [ '-n', '2', lpf.full_path(), '3' ], timeout: test_timeout, suite: testSuite )
endif
