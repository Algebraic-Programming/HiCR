testSuite = [ 'examples', 'hicr', 'toplogy', 'local' ]
test_timeout = 60

if 'hwloc' in enabledBackends
	hwloc = executable('hwloc', [ 'source/hwloc.cpp'], dependencies: hicrBuildDep, include_directories: ['source'] )

	if get_option('buildTests')
	  test('hlwoc', hwloc, timeout: test_timeout, suite: testSuite )
	endif
endif

if 'acl' in enabledBackends 
	acl = executable('acl', [ 'source/acl.cpp'], dependencies: hicrBuildDep, include_directories: ['source'] )

	if get_option('buildTests')
	  test('acl', acl, timeout: test_timeout, suite: testSuite )
	endif
endif

if 'opencl' in enabledBackends 
	opencl = executable('opencl', [ 'source/opencl.cpp'], dependencies: hicrBuildDep, include_directories: ['source'] )

	if get_option('buildTests')
	  test('opencl', opencl, timeout: test_timeout, suite: testSuite )
	endif
endif
