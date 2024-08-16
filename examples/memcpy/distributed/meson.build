testSuite = [ 'examples', 'hicr', 'memcpy' ]
test_timeout = 60

if 'mpi' in enabledBackends and 'host/hwloc' in enabledBackends 
	mpi = executable('mpi', [ 'source/mpi.cpp'], dependencies: hicrBuildDep)
	if get_option('buildTests')
	  test('mpi', mpirunExecutable, args : [ '-np', '2', mpi.full_path() ], timeout: test_timeout, suite: testSuite )
	endif
endif

if 'lpf' in enabledBackends and 'host/hwloc' in enabledBackends 
	lpf = executable('lpf', [ 'source/lpf.cpp'], dependencies: hicrBuildDep)

	if get_option('buildTests')
	  test('lpf', lpfrunExecutable, args : [ '-np', '2', '-engine', 'ibverbs', lpf.full_path() ], timeout: test_timeout, suite: testSuite ) 
	endif
endif

