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

if 'ascend' in enabledBackends and 'hwloc' in enabledBackends
	ascend = executable(
		'ascend',
		[
			'ascend.cpp',
			'include/factory/executionUnit/ascend/executionUnitFactory.cpp',
			'include/tensor/ascend/tensor.cpp',
		],
		dependencies: [NNHiCRBuildDep],
	)
	test(
		'ascend',
		ascend,
		args: [
			modelPath,
			testsetDir,
			kernelsPath,
			testsetDir + '/labels.bin',
			'50'
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
			kernelsPath,
			testsetDir + '/labels.bin',
			'50'
		],
		timeout: 600,
		suite: testSuite,
	)
endif