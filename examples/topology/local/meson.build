testSuite = [ 'examples', 'hicr', 'toplogy', 'local' ]
test_timeout = 60

if 'host/hwloc' in enabledBackends
	hwloc = executable('hwloc', [ 'source/hwloc.cpp'], dependencies: hicrBuildDep, include_directories: ['source'] )

	if get_option('buildTests')
	  test('hlwoc', hwloc, timeout: test_timeout, suite: testSuite )
	endif
endif

if 'ascend' in enabledBackends 
	ascend = executable('ascend', [ 'source/ascend.cpp'], dependencies: hicrBuildDep, include_directories: ['source'] )

	if get_option('buildTests')
	  test('ascend', ascend, timeout: test_timeout, suite: testSuite )
	endif
endif
