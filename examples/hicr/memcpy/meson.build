testSuite = [ 'examples', 'hicr', 'memcpy' ]
test_timeout = 60

if 'host/hwloc' in enabledBackends and 'host/pthreads' in enabledBackends
	sharedMemory = executable('sharedMemory', [ 'source/sharedMemory.cpp'], dependencies: hicrBuildDep )

	if get_option('buildTests')
	  test('sharedMemory', sharedMemory, args : [ ], timeout: test_timeout, suite: testSuite )
	endif
endif

if 'mpi' in enabledBackends and 'host/hwloc' in enabledBackends 
	mpi = executable('mpi', [ 'source/mpi.cpp'], dependencies: hicrBuildDep)
	if get_option('buildTests')
	  test('mpi', mpirunExecutable, args : [ '-np', '2', mpi.full_path() ], timeout: test_timeout, suite: testSuite )
	endif
endif

if 'lpf' in enabledBackends and 'host/hwloc' in enabledBackends 
	lpf = executable('lpf', [ 'source/lpf.cpp'], dependencies: hicrBuildDep)
	lpf2 = executable('lpfMemcpyAndCountingFence', [ 'source/lpfMemcpyAndCountingFence.cpp'], dependencies: hicrBuildDep)

	if get_option('buildTests')
	  test('lpf', lpfrunExecutable, args : [ '-np', '2', '-engine', 'zero', lpf.full_path() ], timeout: test_timeout, suite: testSuite ) 
	endif
endif

if 'ascend' in enabledBackends and 'host/hwloc' in enabledBackends
	ascend = executable('ascend', [ 'source/ascend.cpp'], dependencies: hicrBuildDep)

	if get_option('buildTests')
	  test('ascend', ascend, args : [ ], is_parallel: false, timeout: test_timeout, suite: testSuite ) 
	endif
endif
