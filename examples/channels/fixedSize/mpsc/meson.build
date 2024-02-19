testSuite = [ 'examples', 'channels', 'mpsc']
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
	mpi2 = executable('mpi2', [ 'source/mpi2.cpp' ], dependencies: hicrBuildDep, include_directories: [exampleBuildIncludes] )

	if get_option('buildTests')
	test('mpi', mpirunExecutable, args : [ '-n', '16', mpi.full_path(), '3' ], timeout: test_timeout, suite: testSuite )
	endif
endif
