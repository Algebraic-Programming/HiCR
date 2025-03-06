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

	# ascend
	if 'ascend' in enabledBackends
		ascend = executable('ascend', [ 'source/ascend.cpp'], dependencies: hicrBuildDep)

		if get_option('buildTests')
		test('ascend', ascend, args : [ ], is_parallel: false, suite: testSuite )
		endif
	endif
endif

