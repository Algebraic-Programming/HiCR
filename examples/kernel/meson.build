testSuite = [ 'examples', 'hicr', 'kernel' ]

if 'hwloc' in enabledBackends and 'pthreads' in enabledBackends
	# pthreads
	if 'pthreads' in enabledBackends
		openblasDep = dependency('openblas', required: true)

		pthreads = executable('pthreads', [ 'source/pthreads.cpp' ], dependencies: [hicrBuildDep, openblasDep])

		if get_option('buildTests')
		test('pthreads', pthreads, suite: testSuite )
		endif	
	endif

	# nosv
	if 'nosv' in enabledBackends
		openblasDep = dependency('openblas', required: true)

		nosv = executable('nosv', [ 'source/nosv.cpp'], dependencies: [hicrBuildDep, openblasDep])

		if get_option('buildTests')
		test('nosv', nosv, args : [ ], is_parallel: false, suite: testSuite )
		endif
	endif

	# acl
	if 'acl' in enabledBackends
		acl = executable('acl', [ 'source/acl.cpp'], dependencies: hicrBuildDep)

		if get_option('buildTests')
		test('acl', acl, args : [ ], is_parallel: false, suite: testSuite )
		endif
	endif
endif

if 'opencl' in enabledBackends and 'hwloc' in enabledBackends
	opencl = executable('opencl', [ 'source/opencl.cpp'], dependencies: hicrBuildDep)

	if get_option('buildTests')
	  test('opencl', opencl, args : [ meson.current_source_dir() + '/kernels/gemm.cl' ], is_parallel: false, timeout: test_timeout, suite: testSuite ) 
	endif
endif


