if 'pthreads' in enabledBackends and 'hwloc' in enabledBackends
	pthreads = executable(
		'pthreads',
		[
			'pthreads.cpp',
			'include/factory/executionUnit/pthreads/executionUnitFactory.cpp',
			'include/tensor/pthreads/tensor.cpp',
		],
		dependencies: [NNHiCRBuildDep],
	)
	test(
		'pthreads',
		pthreads,
		args: [
			modelPath,
			testsetDir,
			testsetDir + '/labels.bin',
			'50'
		],
		timeout: 600,
		suite: testSuite,
	)
endif

if 'acl' in enabledBackends and 'hwloc' in enabledBackends
	acl = executable(
		'acl',
		[
			'acl.cpp',
			'include/factory/executionUnit/acl/executionUnitFactory.cpp',
			'include/tensor/acl/tensor.cpp',
		],
		dependencies: [NNHiCRBuildDep],
	)
	test(
		'acl',
		acl,
		args: [
			modelPath,
			testsetDir,	
			testsetDir + '/labels.bin',
			'50',
			kernelsPath
		],
		timeout: 600,
		suite: testSuite,
	)
endif

if 'opencl' in enabledBackends and 'hwloc' in enabledBackends
	opencl = executable(
		'opencl',
		[
			'opencl.cpp',
			'include/factory/executionUnit/opencl/executionUnitFactory.cpp',
			'include/tensor/opencl/tensor.cpp',
		],
		dependencies: [NNHiCRBuildDep],
	)
	test(
		'opencl',
		opencl,
		args: [
			modelPath,
			testsetDir,
			testsetDir + '/labels.bin',
			'50',
			kernelsPath
		],
		timeout: 600,
		suite: testSuite,
	)
endif