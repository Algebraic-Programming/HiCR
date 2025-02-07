testSuite = [ 'examples', 'channels', 'variableSize', 'mpsc', 'nonlocking']
test_timeout = 60

exampleBuildIncludes = include_directories([
   'include'
  ])

if 'mpi' in enabledBackends and 'hwloc' in enabledBackends
	mpi = executable('mpi', [ 'source/mpi.cpp' ], dependencies: hicrBuildDep, include_directories: [exampleBuildIncludes] )

	if get_option('buildTests')
	  test('mpi', mpirunExecutable, args : [ '-n', '8', mpi.full_path(), '256' ], timeout: test_timeout, suite: testSuite )
    endif
endif

if 'lpf' in enabledBackends and 'hwloc' in enabledBackends
	lpf = executable('lpf', [ 'source/lpf.cpp' ], dependencies: hicrBuildDep, include_directories: [exampleBuildIncludes] )

	if get_option('buildTests')
	  test('lpf', lpfrunExecutable, args : [ '-np', '8', '-engine', 'zero', lpf.full_path(), '256' ], timeout: test_timeout, suite: testSuite )
	endif
endif
