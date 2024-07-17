testSuite = [ 'examples', 'hicr', 'kernel' ]

if 'host/hwloc' in enabledBackends and 'host/pthreads' in enabledBackends
	pthreads = executable('pthreads', [ 'source/pthreads.cpp' ], dependencies: hicrBuildDep)

	if get_option('buildTests')
	 test('pthreads', pthreads, suite: testSuite )
	endif
endif

if 'ascend' in enabledBackends and 'host/hwloc' in enabledBackends
	ascend = executable('ascend', [ 'source/ascend.cpp'], dependencies: hicrBuildDep)

	if get_option('buildTests')
	  test('ascend', ascend, args : [ ], is_parallel: false, suite: testSuite )
	endif
endif
