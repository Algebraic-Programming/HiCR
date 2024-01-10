testSuite = [ 'examples', 'hicr', 'toplogy' ]
test_timeout = 60

if 'hwloc' in enabledBackends
	hwloc = executable('hwloc', [ 'source/hwloc.cpp'], dependencies: [ hicrBuildDep, hicrHWLocBackendDep ], include_directories: ['source'] )

	if get_option('buildTests')
	  test('hlwoc', hwloc, timeout: test_timeout, suite: testSuite )
	endif
endif

if 'ascend' in enabledBackends 
	hwloc = executable('ascend', [ 'source/ascend.cpp'], dependencies: [ hicrBuildDep, hicrAscendBackendDep ], include_directories: ['source'] )

	if get_option('buildTests')
	  test('ascend', ascend, timeout: test_timeout, suite: testSuite )
	endif
endif
